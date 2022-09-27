#include "kernel/types.h"
#include "user.h"

void sub_fork(int p_read){
    int prime;
    if (read(p_read, &prime, 4) !=0){
        int current_fd[2];
        pipe(current_fd);
        printf("prime %d\n", prime);
        if (fork()==0){
            close(current_fd[1]);       //子进程在读之前要关闭通道 
            sub_fork(current_fd[0]);
        }
        else
        {
            int temp;
            while (read(p_read, &temp, 4) != 0){
                if(temp % prime != 0){
                    write(current_fd[1], &temp, 4);
                }
            }
            close(current_fd[1]);       //父进程在写完要关闭通道 
            wait(0);
        }
    }
    exit(0);
}

int main()
{
    int p[2];
    pipe(p);
    if(fork() == 0){
        close(p[1]);
        sub_fork(p[0]);
    }else{
        for (int i = 2; i < 36; i++)
        {
            write(p[1], &i, 4);
        }
        close(p[1]);
        wait(0);
    }
    exit(0);
}
