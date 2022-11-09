#include "kernel/types.h"
#include "user/user.h"
#include "stddef.h"

//两个参数分别为两个文件描述符
//通过操作文件描述符实现重定向,减少资源消耗
void redirection(int n, int fd[]){
    //关闭n
    close(n);
    //产生一个从n到fd[n]的映射
    dup(fd[n]);
    //关闭管道中描述符
    close(fd[1]);
    close(fd[0]);
    
}

//利用重定向筛选质数
//notice: pipe is a part of buffer zone!!
void primes(){
    //file descriptor needed
    int fd[2];
    //int buf
    //not sure of the size of previous and follow, use & to link address
    int previous,follow;
    if(read(0, &previous, sizeof(int))){
        printf("prime %d\n",previous);
        //1 pipe is enough!
        pipe(fd);
        if(fork() == 0){
            /*child process*/
            //link writing to file director 1
            redirection(1,fd);
            /*discarding all those are the multiplies of previous*/
            while(read(0, &follow, sizeof(int))){
                if(follow % previous != 0){
                    write(1, &follow, sizeof(int));
                }
            }
        }else{
            /*parent process*/
            //wait until child finishes processing data
            wait(NULL);
            redirection(0,fd);
            //do it recursively(the next time, previous will move forward)
            primes();
        }
    }
    
}

int main(int argc,char *argv[]){
    int fd[2];
    pipe(fd);
    if(fork() == 0){
        /*child process*/
        //write 2-35 into the pipe
        redirection(1,fd);
        for(int i = 2; i < 36; i++){
            write(1, &i, sizeof(int));
        }
    }else{
        /*parent process*/
        wait(NULL);
        redirection(0, fd);
        primes();
    }
    exit(0);
}