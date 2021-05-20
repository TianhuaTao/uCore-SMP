#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int pid1 = fork();
    if (pid1 == 0)
    {
        exec("test_shmem1");
    }
    int pid2 = fork();
    if (pid2 == 0)
    {
        exec("test_shmem2");
    }
    waitpid(pid1, NULL);
    waitpid(pid2, NULL);
    return 0;
}