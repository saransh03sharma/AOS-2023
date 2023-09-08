#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

void execute(int val[], int n) {
    int fd = open("/proc/partb_1_20CS10085_20CS30065", O_RDWR);
    char c = (char)n;
    
    write(fd, &c, 1);

    
    int ret = write(fd, &val[0], sizeof(int));
    printf("[Proc %d] Write: %d, Return: %d\n", getpid(), val[0], ret);
    usleep(100);
    

    ret = write(fd, &val[1], sizeof(int));
    printf("[Proc %d] Write: %d, Return: %d\n", getpid(), val[1], ret);
    usleep(100);

    ret = write(fd, &val[2], sizeof(int));
    printf("[Proc %d] Write: %d, Return: %d\n", getpid(), val[2], ret);
    usleep(100);

    ret = write(fd, &val[3], sizeof(int));
    printf("[Proc %d] Write: %d, Return: %d\n", getpid(), val[3], ret);
    usleep(100);

    

    int out;
    ret = read(fd, &out, sizeof(int));
    printf("[Proc %d] Read: %d, Return: %d\n", getpid(), out, ret);
    usleep(100);

    ret = write(fd, &val[4], sizeof(int));
    printf("[Proc %d] Write: %d, Return: %d\n", getpid(), val[4], ret);
    usleep(100);

    int x=7;
    ret = write(fd,&x , sizeof(int));
    printf("[Proc %d] Write: %d, Return: %d\n", getpid(), x, ret);
    usleep(100);
    
    for(int i=0;i<n;++i)
    {
        int out;
        int ret = read(fd, &out, sizeof(int));
        printf("[Proc %d] Read: %d, Return: %d\n", getpid(), out, ret);
        usleep(100);
    
    }
    
    close(fd);
}

int main(void){
    int val_p[] = {5, 1, -2, 3, 4};
    execute(val_p, 5);
    return 0;
}