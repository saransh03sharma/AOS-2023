#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

void execute(int val[], int n) {
    int fd = open("/proc/partb_1_20CS10085_20CS30065", O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
    char c = (char)n;
    ssize_t ret = write(fd, &c, 1);
    if (ret == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < n; i++) {
        val[i] += getpid(); // Add PID to the value
    }

    for (int i = 0; i < n; i++) {
        ret = write(fd, &val[i], sizeof(int));
        if (ret == -1) {
            perror("write");
            close(fd);
            exit(EXIT_FAILURE);
        }
        printf("[Proc %d] Write: %d, Return: %zd\n", getpid(), val[i], ret);
        usleep(100);
    }
    
    for(int i = 0; i < n; ++i) {
        int out;
        ret = read(fd, &out, sizeof(int));
        if (ret == -1) {
            perror("read");
            close(fd);
            exit(EXIT_FAILURE);
        }
        printf("[Proc %d] Read: %d, Return: %zd\n", getpid(), out, ret);
        usleep(100);
    }
    
    close(fd);
}

int main(void) {
    int val_p[] = {0, 1, -2, 3, 4};
    
    // Create a child process
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        execute(val_p, 5);
    } else {
        // Parent process
        execute(val_p, 5);
        wait(NULL); // Wait for the child process to finish
    }

    return 0;
}
