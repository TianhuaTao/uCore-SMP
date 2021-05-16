#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>

const int MAX_CHILD = 40;

int main() {
    for (int i = 0; i < MAX_CHILD; ++i) {
        int pid = fork();
        if (pid == 0) {
            printf("I am child %d\n", i);
            exit(0);
        } else {
            printf("forked child pid = %d\n", pid);
        }
    }

    int exit_code = 0;
    for (int i = 0; i < MAX_CHILD; ++i) {
        pid_t p = wait(&exit_code);
        if (p <= 0) {
            panic("wait stopped early");
        }
        printf("child %d exited with code %d\n", p, exit_code);
    }
    if (wait(&exit_code) > 0) {
        panic("wait got too many");
    }
    printf("forktest pass.\n");
    return 0;
}