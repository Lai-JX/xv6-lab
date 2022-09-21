#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    // printf("ii");
    char command[256], *argv_[MAXARG];
    int p_c=0, p_a=0;
    // 读取要执行的命令, 保留除第一个参数外的所有参数的字母到command，参数指针保留到argv_
    for (int i = 1; i < argc; i++)
    {
        argv_[p_a++] = command + p_c;
        for (int j = 0; j < strlen(argv[i]); j++)
        {
            command[p_c++] = argv[i][j];
        }
        command[p_c++] = '\0';
    }
    argv_[p_a++] = command + p_c;
    int p_c1 = p_c;     // 将读取完命令行的p_c 保留
    int p_a1 = p_a;     // 将读取完命令行的p_a 保留
    char ch;
    while (read(0, &ch, 1)>0)       // 读取字符，每次读取一个
    {
        // printf("%c\n", ch);
        if (ch == '\n')
        {
            // int i = 0;
            // while (argv_[i] != 0)
            // {
            //     printf("sub:%s\n", argv_[i]);
            //     i++;
            // }
            // printf("\n");
            command[p_c++] = '\0';
            argv_[p_a++] = 0;
            if (fork()==0)
            {
                exec(argv[1], argv_);
            }
            else        // 在父进程中，将参数回退到原本的状态
            {
                wait(0);
                p_a = p_a1;
                p_c = p_c1;
                
            }
        }
        else if (ch == ' ')
        {
            command[p_c++] = '\0';
            argv_[p_a++] = command + p_c;
        }
        else
        {
            command[p_c++] = ch;
        }
        // int i = 0;
        // while (argv_[i] != 0)
        // {
        //     printf("out:%s\n", argv_[i]);
        //     i++;
        // }
            // printf("\n");
        
        // printf("%s", argv[1]);
    }
    exit(0);
}
