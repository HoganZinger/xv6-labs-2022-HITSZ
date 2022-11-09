#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//using part of the code in ls.c

void
find(char *path, char *file)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        close(fd);
        fprintf(2, "find: cannot stat %s\n", path);
        return;
    }

    //directory name too long
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        close(fd);
        fprintf(2, "find: directory name is too long!\n");
        return;
    }
    //not a directory
    if(st.type != T_DIR){
        close(fd);
        fprintf(2, "find:%s is not a directory\n", path);
        return;
    }
    
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0){
            continue;
            }
        if(!strcmp(de.name,".") || !strcmp(de.name, "..")){
            continue;
        }
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        if(stat(buf, &st) < 0){
            fprintf(2, "find: can't stat %s\n", buf);
            continue;
        }

        if(st.type == T_DIR){
            find(buf, file);
        }
        else if(st.type == T_FILE && !strcmp(de.name, file)){
            printf("%s\n", buf);
        }
    }

 
}

int
main(int argc, char *argv[])
{
    
    if(argc != 3){
        fprintf(2, "usage: find dirName fileName\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}
