#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>

const int NUM = 30;

int main() {
    for(int i = 0; i < NUM; ++i) {
        int pid = fork();
        if(pid == 0) {
            int current_time = time_ms();
            unsigned long long sleep_length = current_time * current_time % 1000 + 1000;
            sleep(sleep_length);
            printf("pid %d OK!\n", getpid());
            exit(0);
        }
    }
    
    int xstate = 0;
    for(int i = 0; i < NUM; ++i) {
        assert(waitpid(-1, &xstate) > 0, -1);
        assert(xstate == 0, -2);
    }
    assert(waitpid(-1, &xstate) < 0, -3);
    printf("forktest2 test passed!\n");
    return 0;
}
