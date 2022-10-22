// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13
struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  //struct buf head;
  struct buf hashbucket[NBUCKETS]; //每个哈希队列一个linked list及一个lock
} bcache;

void
binit(void)
{
  struct buf *b;

  // 初始化锁和链表头
  for (int i = 0; i < NBUCKETS; i++)
  {
    initlock(&bcache.lock[i], "bcache");
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }
      
  // Create linked list of buffers
  int count = 0;
  int id = 0;
  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    id = count % NBUCKETS;
    b->next = bcache.hashbucket[id].next;
    b->prev = &bcache.hashbucket[id];

    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[id].next->prev = b;
    bcache.hashbucket[id].next = b;

    count++;
  }
}

// 根据哈希表编号查找该哈希桶是否有空闲的buf，有则返回，没有则返回 0
// Lock_flag为1表示加锁，Lock_flag为0表示不加锁
struct buf*
bfind(int hash_id, int Lock_flag)
{
  struct buf *b;
  if (Lock_flag)
    acquire(&bcache.lock[hash_id]);
  for(b = bcache.hashbucket[hash_id].prev; b != &bcache.hashbucket[hash_id]; b = b->prev){
    if(b->refcnt == 0) {
      // 将b从双向链表取出
      b->prev->next = b->next;
      b->next->prev = b->prev;
      b->next = 0;
      b->prev = 0;
      if (Lock_flag)
        release(&bcache.lock[hash_id]);
      return b;
    }
  }
  if (Lock_flag)
    release(&bcache.lock[hash_id]);
  return 0;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = blockno % NBUCKETS;
  acquire(&bcache.lock[id]);

  // Is the block already cached?
  for(b = bcache.hashbucket[id].next; b != &bcache.hashbucket[id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.(先在当前哈希桶内获取,若差不到就找下一哈希桶...)
  b = bfind(id,0);    // 查找当前队列
  int temp_hashid = id;
  if (!b) 
    for (int i = 1; i < NBUCKETS; i++)
    {
      temp_hashid = (id + i) % NBUCKETS;
      b = bfind(temp_hashid,1);
      if (b)
        break;
    }

  if (b)
  {
    // 采用头插法插入当前链表
    b->next = bcache.hashbucket[id].next;
    b->prev = &bcache.hashbucket[id];
    bcache.hashbucket[id].next->prev = b;
    bcache.hashbucket[id].next = b;
    
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    release(&bcache.lock[id]);
    acquiresleep(&b->lock);
    return b;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int id = (b->blockno) % NBUCKETS;

  acquire(&bcache.lock[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.(没用则插到head后面)
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[id].next;
    b->prev = &bcache.hashbucket[id];
    bcache.hashbucket[id].next->prev = b;
    bcache.hashbucket[id].next = b;
  }
  
  release(&bcache.lock[id]);
}

void
bpin(struct buf *b) {
  int id = b->blockno % NBUCKETS;
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void
bunpin(struct buf *b) {
  int id = b->blockno % NBUCKETS;
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}


