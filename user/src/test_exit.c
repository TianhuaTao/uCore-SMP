#include <stdio.h>
#include <ucore.h>
const int max_child = 32;

const int MAGIC = 0x1234;

int main(void) {
    puts("I am the parent. Forking the child...");
    int pid = fork();
    if (pid == 0) {
        puts("I am the child.");
        for(int i = 0; i < 8; ++i) {
            sched_yield();
        }
        exit(MAGIC);
    } else {
        printf("I am parent, fork a child pid %d\n", pid);
    }

    puts("I am the parent, waiting now..");
    int xstate = 0;
    if(waitpid(pid, &xstate) != pid || xstate != MAGIC) {
        printf("wait %d fail\n", pid);
    }
    if(waitpid(pid, &xstate) > 0 || wait( &xstate) > 0) {
        printf("wait should fail\n", pid);
    }
    printf("waitpid %d ok.\n", pid);
    puts("exit pass.");
    return 0;
}

