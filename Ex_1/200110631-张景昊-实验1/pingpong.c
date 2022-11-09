#include "kernel/types.h"
#include "user/user.h"
#include "stddef.h"

int main(int argc,char *argv[]){
    int parent_fd[2],child_fd[2];//定义两个文件描述符数组
    pipe(parent_fd);
    pipe(child_fd);//创建两个管道

    if(argc != 1){
        printf("arguments incorrect! argc should be 1!\n");
        exit(-1);
    }
    if(pipe(parent_fd) == -1 || pipe(child_fd) == -1){
        printf("Pipe create failed!\n");
        exit(-1);
    }

    char buf[8];
    if(fork() == 0){
        /*子进程*/
        close(parent_fd[1]);
        read(parent_fd[0],buf,4);
        printf("%d: received %s\n",getpid(),buf);
        close(parent_fd[0]);
        close(child_fd[0]);
        write(child_fd[1],"pong",strlen("pong"));
        close(child_fd[1]);
    }else //if(fork() > 0){
    {   /*父进程*/

        write(parent_fd[1],"ping",strlen("ping"));
        close(parent_fd[1]);
        wait(NULL);//等待子进程
        read(child_fd[0],buf,4);
        printf("%d: received %s\n",getpid(),buf);
        close(child_fd[0]);
        
    }
    exit(0);
 

}

