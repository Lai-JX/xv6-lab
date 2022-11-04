#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* fmtname(char *path) // 返回文件名
{
    char *p;
    // Find first character after last slash.（斜线）；找到最后一个斜线即可找到文件名
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;
    return p;
}

void find(char *path, char *name)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){     // 打开目录文件，成功返回0，失败返回-1
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){   // fstat 将fd指向的文件状态复制到st指向的结构体中，成功返回0，失败返回-1
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type)  // 1表示文件夹，2表示文件
    {
    case T_FILE:
        if(strcmp(name, fmtname(path))==0){     // 比对文件名，查看是否为要找的文件
            printf("%s\n", path);
        }
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

        while (read(fd, &de, sizeof(de)) == sizeof(de))
        { // fd指向一个目录文件，该目录文件包含dirent序列
            // de.name为文件名，de.inum为文件标识符，为0说明未分配文件标识符，跳过
            if (de.inum == 0)
                continue;

            // 不要递归进入.和..
            if (strcmp(de.name, ".")==0 || strcmp(de.name, "..")==0)
                continue;

            // path/后面追加文件名，相当于path/file_name(或下一级路径)
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            
            find(buf, name);
        }
        break;
    }
    close(fd);
} 

int main(int argc, char *argv[])
{
    if(argc < 3){
        printf("find:The number of parameters is incorrect\n");
    }
    find(argv[1], argv[2]);
    exit(0);
}
