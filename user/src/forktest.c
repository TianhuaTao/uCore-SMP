#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
        if (wait(-1, &exit_code) <= 0) {
            panic("wait stopped early");
        }
    }
    if (wait(-1, &exit_code) > 0) {
        panic("wait got too many");
    }
    printf("forktest pass.\n");
    return 0;
}