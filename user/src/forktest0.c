#include <stdio.h>
#include <ucore.h>
#include <stdlib.h>

int main() {
    assert(wait( NULL) < 0, -1);
    printf("sys_wait without child process test passed!\n");
    printf("parent start, pid = %d!\n", getpid());
    int pid = fork();
    if(pid == 0) {
        // child process
        printf("hello child process!\n");
        return 100;
    } else {
        // parent process
        int xstate = 0;
        printf("ready waiting on parent process!\n");
        assert(pid == wait( &xstate), -2);
        assert(xstate == 100, -3);
        printf("child process pid = %d, exit code = %d\n", pid, xstate);
    }
    return 0;
}