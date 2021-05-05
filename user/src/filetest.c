#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main() {
    int exit_code;
    int fd = open("test\0", O_CREATE | O_WRONLY, O_RDWR);
    printf("open OK, fd = %d\n", fd);
    char str[100] = "hello world!\0";
    int len = strlen(str);
    write(fd, str, len);
    close(fd);
    puts("write over.");
    if(fork() == 0) {
        int fd = open("test\0", O_RDONLY, O_RDWR);
        char str[100];
        str[read(fd, str, len)] = 0;
        puts(str);
        puts("read over.");
        close(fd);
        exit(0);
    }
    wait(-1, &exit_code);
    puts("filetest passed.");
    return 0;
}