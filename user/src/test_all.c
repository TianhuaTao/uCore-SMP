#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ucore.h>

const char *testcases[] = {
    "test_exec",
    "test_exit",
    "test_pipe",
    "test_main",
    "matrix",
    "test_file",
};

int run_testcase(const char *testcase, int n, int all_cnt) {
    int pid = fork();
    if (pid == 0) {
        // child
        exec(testcase);
        printf("\x1b[31m[FAILED]\x1b[0m exec() error\n");
    } else if (pid < 0) {
        printf("fork() error\n");
    } else {
        // parent
        int exit_code;
        int wait_pid = waitpid(pid, &exit_code);
        if (wait_pid <= 0) {
            printf("\x1b[31m[FAILED]\x1b[0m wait() error\n");
            return 1;
        }

        if (exit_code == 0) {
            printf("\x1b[32m[PASSED %d/%d]\x1b[0m %s\n", n, all_cnt, testcase);
            return 0;
        } else {
            printf("\x1b[31m[FAILED %d/%d]\x1b[0m %s\n", n, all_cnt, testcase);
            return 1;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    printf("Running all tests on uCore-SMP\n");

    int n_testcase = sizeof(testcases) / sizeof(testcases[0]);
    int failed = 0;
    for (int i = 0; i < n_testcase; i++) {
        failed += run_testcase(testcases[i], i + 1, n_testcase);
    }
    if (failed > 0) {
        printf("Tests passed: %d/%d\n", n_testcase - failed, n_testcase);
    } else {
        printf("\x1b[32m[PASSED]\x1b[0m All tests passed\n");
    }
    return 0;
}
