#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <fcntl.h>
#include <string.h>

int main()
{
    int ppid;
    int cpid;
    ppid = getpid();
    printf("parent get parent pid = %d\n", ppid);
    int exit_code;
    cpid = fork();
    if (cpid == 0)
    {
        // child
        int t = getppid();
        int t2 = getpid();
        printf("child get parent pid = %d\n", t);
        printf("child get child pid = %d\n", t2);
        assert(t == ppid, -2);
    }
    else
    {
        wait(&exit_code);
        printf("parent get child pid = %d\n", cpid);
        assert(exit_code == 0,-4);
    }

    return 0;
}