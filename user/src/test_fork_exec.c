#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>

#define PROC_NUM 10

int main()
{
    int pid[PROC_NUM], ret[PROC_NUM];
    puts("execute prime");
    for (int i = 0; i < PROC_NUM; ++i)
    {
        int tmp_pid = fork();
        if (tmp_pid == 0)
        {
            sched_yield();
            exec("prime");
        }
        else
        {
            set_dsid(tmp_pid, 0);
            pid[i] = tmp_pid;
        }
    }
    for (int i = 0; i < PROC_NUM; ++i)
        waitpid(pid[i], &ret[i]);
    return 0;
}