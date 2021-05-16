#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <fcntl.h>
#include <string.h>

int main() {
    int exit_code;
    int fd = open("test", O_CREATE | O_WRONLY);
    printf("open OK, fd = %d\n", fd);
    char str[100] = "hello world!\0";
    int len = strlen(str);
    write(fd, str, len);
    close(fd);
    puts("write over.");
    if(fork() == 0) {
        int fd = open("test", O_RDONLY);
        char str[100];
        str[read(fd, str, len)] = 0;
        puts(str);
        puts("read over.");
        close(fd);
        exit(0);
    }
    wait(&exit_code);
    puts("filetest passed.");
    return 0;
}