#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.（斜线）
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));   // 将p拷贝到buf
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));//将指针 buf+strlen(p) 所指向的前 DIRSIZ-strlen(p) 字节的内存单元替换为空。这里是要使buf用空格填满到长度为DIRSIZ
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){     // 打开目录文件，成功返回0，失败返回-1
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){   // fstat 将fd指向的文件状态复制到st指向的结构体中，成功返回0，失败返回-1
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    // 将path复制到buf
    strcpy(buf, path);
    // 指针p指向buf最后
    p = buf+strlen(buf);
    // 追加“/”，相当于path/
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){// fd指向一个目录文件，该目录文件包含dirent序列
      // de.name为文件名，de.inum为文件标识符，为0说明未分配文件标识符，跳过
      if(de.inum == 0)
        continue;
      // path/后面追加文件名，相当于path/file_name
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      // 匹配到无效文件
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit(0);
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit(0);
}
