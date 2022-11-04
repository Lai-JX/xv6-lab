// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU];
int init_flag = 0;  // 标注是否在初始化
int init_cpu_id = 0;  // 正在初始化的cpu的id

void
kinit()
{
  // 只有当首次启动时会被调用这个函数，故可以在这里将freelist分为每个CPU独立的freelist

  // 每个cpu的空间(均分) 以PFSIZE为单位
  uint64 cpu_kmem_size = (PHYSTOP - (uint64)end) / (NCPU*PGSIZE);

  init_flag = 1;  // 标志为初始化
  // 遍历各个CPU，设置锁并分配freelist
  for (int i = 0; i < NCPU; i++)
  {
    init_cpu_id = i;
    initlock(&kmems[i].lock, "kmem");
    uint64 base = (uint64)end + cpu_kmem_size * i*PGSIZE;

    if (i == NCPU-1)
      freerange((void*)base, (void*)PHYSTOP);
    else
      freerange((void*)base, (void*)(base + cpu_kmem_size*PGSIZE));
  }
  init_flag = 0;  // 初始化完毕
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;


  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  int cpu_id;
  if (init_flag == 1)       // 如果是初始化，则采用要分配内存的cpu_id
    cpu_id = init_cpu_id;
  else                      // 回收到当前CPU对应的空闲页链表
  {
    push_off();
    cpu_id = cpuid();
    pop_off();
  }

  acquire(&kmems[cpu_id].lock);
  r->next = kmems[cpu_id].freelist;   // 采用头插法插入空闲页链表
  kmems[cpu_id].freelist = r;
  release(&kmems[cpu_id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int cpu_id = cpuid();
  pop_off();

  for (int i = 0; i < NCPU; i++)
  {
    int id = (cpu_id + i) % NCPU;    // 优先访问本CPU的freelist

    acquire(&kmems[id].lock);
    r = kmems[id].freelist;
    if(r)
    {
      kmems[id].freelist = r->next; // 取出表头元素
      release(&kmems[id].lock);
      break;
    }
    else 
      release(&kmems[id].lock);
  }
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  return (void*)r;
}
