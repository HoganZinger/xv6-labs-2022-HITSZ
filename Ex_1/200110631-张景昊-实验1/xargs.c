#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    
    if (argc < 2) {
       
        printf("arguments incorrect! argc should not be less than 2!");
        exit(-1);
    }

    // and indexes needed and buffer zone
    char buffer[128];
    char *argvs[MAXARG] = {"\0"};
    int index = 0;
    int pid;
    for(int i = 1; i < argc; i++){
        char * temp_buf = (char *) malloc(strlen(argv[1]) * sizeof(char));
        temp_buf = strcpy(temp_buf,argv[1]);

        argvs[index] = argv[i];
        index ++;
    }
    
    int n ;
    while((n=read(0, buffer, sizeof(buffer)))> 0){
        int i = 0;
        // read a line
        while(buffer[i]!='\0'){
            int j = 0;
            char *temp_buf = (char *) malloc(sizeof(char) * 128 ) ; 
            while(buffer[i]!='\n'){
                temp_buf[j] = buffer[i];
                i++;
                j++;
            }

            temp_buf[j] = '\0';
            i++;
            if(!(pid = fork())){
                argvs[index] = temp_buf;
                argvs[index+1] = 0;
                exec(argv[1], argvs);
                exit(0);
            }

         wait(&pid);

        }
    }
        
    exit(0);
}


