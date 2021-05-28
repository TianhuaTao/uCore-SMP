#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucore.h>
#include <dsid.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define NCPU 4
#define NDSID 8

char *procstate_str[] = {
    [UNUSED] = "UNUSED",
    [USED] = "USED",
    [SLEEPING] = "SLEEPING",
    [RUNNABLE] = "RUNNABLE",
    [RUNNING] = "RUNNING",
    [ZOMBIE] = "ZOMBIE",
    [PROCSTATE_NUM] = "UNKNOWN",
};

int main(void)
{
    struct cpu_stat stat_buf[8];
    struct mem_stat mstat;
    struct proc_stat pstat[20];
    int fd_cpu = open("cpu", O_RDWR);
    int fd_mem = open("mem", O_RDWR);
    int fd_proc = open("proc", O_RDWR);
    assert(fd_cpu >= 0, -1);

    int64 prev_l2_traffic[NDSID];
    for (int i = 0; i < NDSID; ++i)
        prev_l2_traffic[i] = get_l2_traffic(i);
    for (int time_sec = 1; time_sec <= 10; ++time_sec)
    {
        uint64 start_time = time_ms();
        int bytes_cpu = read(fd_cpu, stat_buf, sizeof(stat_buf));
        int bytes_mem = read(fd_mem, &mstat, sizeof(mstat));
        int bytes_proc = read(fd_proc, &pstat, sizeof(pstat));
        int p_cnt = bytes_proc / sizeof(struct proc_stat);
        assert(bytes_cpu == sizeof(struct cpu_stat) * NCPU, -6); // only four
        assert(bytes_mem == sizeof(struct mem_stat), -8);        // only four
        printf("\x1b[2J");                                       // clear
        printf("----------------------------------------------------------------------\n");
        printf(" uCore-SMP Resource Monitor                            Time: %l s\n", time_sec);
        printf("----------------------------------------------------------------------\n\n");

        printf("CPU\n");
        for (int i = 0; i < NCPU; i++)
        {
            int per = (100 * stat_buf[i].sample_busy_duration) / stat_buf[i].sample_duration;
            printf("[\x1b[31m");
            int tens = (per + 1) / 2;
            for (int j = 0; j < tens; j++)
            {
                printf("|");
            }
            printf("\x1b[%d;52f", i + 6);
            printf("\x1b[0m]  Core  %d    %d%%\n", i, per);
        }

        printf("\n");
        printf("Memory\n");
        printf("Total :   %l MB        ", mstat.physical_total / 1024 / 1024);
        printf("Free  :   %l MB\n", mstat.physical_free / 1024 / 1024);
        uint64 used = mstat.physical_total - mstat.physical_free;
        int use_per = (used * 100) / mstat.physical_total;
        printf("[\x1b[31m");
        int tens = (use_per + 1) / 2;
        for (int i = 0; i < tens; i++)
        {
            printf("|");
        }

        printf("\x1b[13;52f");
        printf("\x1b[0m] %d%%\n\n", use_per);
// Process | pid | ppid | dsid | heap | mem | cpu time | state
        printf("Process | pid | ppid | dsid | heap | mem | cpu time | state\n", p_cnt);
        if (p_cnt > 0)
        {
            for (int i = 0; i < p_cnt; i++)
            {
                int line = i + 16;
                printf("%s", pstat[i].name);
                printf("\x1b[%d;11f", line);
                printf("%d", pstat[i].pid);
                printf("\x1b[%d;17f", line);
                printf("%d", pstat[i].ppid);
                printf("\x1b[%d;24f", line);
                printf("%d", pstat[i].dsid);
                printf("\x1b[%d;31f", line);
                printf("%l", pstat[i].heap_sz);
                printf("\x1b[%d;38f", line);
                printf("%l", pstat[i].total_size / 1024);
                printf("\x1b[%d;44f", line);
                printf("%l", pstat[i].cpu_time);
                printf("\x1b[%d;55f", line);
                printf("%s\n", procstate_str[MIN(pstat[i].state, PROCSTATE_NUM)]);
            }
        }
        puts("\n");
        // printf("----------------------------------------------------------------------\n");

        puts("dsid | L1-L2 traffic (KB/s)");
        for (int i = 0; i < NDSID; ++i)
        {
            int dsid = i;
            int64 l2_traffic = get_l2_traffic(dsid);
            if (l2_traffic < prev_l2_traffic[dsid])
                prev_l2_traffic[dsid] -= UINT_MAX;
            int diff = l2_traffic - prev_l2_traffic[dsid];
            if (diff != 0)
            {
                printf("%d      ", dsid);
                printf("%d\n", diff * 8 / 1024);
                prev_l2_traffic[dsid] = l2_traffic;
            }
        }
        puts("\n");

        // printf("----------------------------------------------------------------------\n\n");
        uint64 sleep_time = 1000 - (time_ms() - start_time);
        if (sleep_time > 0)
            sleep(sleep_time);
    }

    return 0;
}