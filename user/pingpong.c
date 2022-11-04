#include "kernel/types.h"
#include "user.h"

int main(){
    int parent_fd[2], sub_fd[2];
    char buffer[12];        // 创建12个字符的空间作为缓冲区
    pipe(parent_fd);
    pipe(sub_fd);
    int fork_ret = fork();
    if (fork_ret == 0)      // 子进程
    {
        // 从管道读取父进程信息
        close(parent_fd[1]);            // 关闭写端
        read(parent_fd[0], buffer, 4);  // 通过文件描述符读取
        close(parent_fd[0]);            // 关闭读端

        // 输出
        buffer[4] = '\0';
        printf("%d: received %s\n", getpid(), buffer);

        // 写入管道
        close(sub_fd[0]);               // 关闭读端
        write(sub_fd[1], "pong", 4);    // 通过文件描述符读取
        close(sub_fd[1]);               // 关闭写端
        exit(0);
    }
    else if (fork_ret > 0)  // 父进程
    {
        // 写入管道
        close(parent_fd[0]);
        write(parent_fd[1], "ping", 4);
        close(parent_fd[1]);

        // 从管道读取子进程信息
        close(sub_fd[1]);
        read(sub_fd[0], buffer, 4);
        close(sub_fd[0]);
        // 输出
        buffer[4] = '\0';
        printf("%d: received %s\n", getpid(), buffer);
        wait(0);                // 等待子进程退出
    }
    exit(0);
}