#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <dsid.h>
#include <ucore.h>

void MonitorTraffic(void)
{
    int64 prev_l2_traffic[4];
    for (int dsid = 0; dsid <= 3; ++dsid)
        prev_l2_traffic[dsid] = get_l2_traffic(dsid);
    for (int i = 0; i < 10; ++i)
    {
        printf("current time(ms): %d | l1 to l2 speed(KB/s) | ", time_ms());
        for (int dsid = 0; dsid <= 3; ++dsid)
        {
            int64 l2_traffic = get_l2_traffic(dsid);
            if (l2_traffic < prev_l2_traffic[dsid])
                prev_l2_traffic[dsid] -= UINT_MAX;
            printf("dsid %d: %d | ", dsid, (l2_traffic - prev_l2_traffic[dsid]) * 8 / 1024);
            prev_l2_traffic[dsid] = l2_traffic;
        }
        putchar('\n');
        sleep(1000);
    }
    return;
}

typedef struct
{
    uint32 bucket_freq, bucket_size, bucket_inc, l2_mask;
} DsidParam;

void GenerateWorkload(const char *name)
{
    printf("generating workload %s\n", name);
    int pid_arr[4], time_usage[4];

    for (int j = 1; j <= 3; ++j)
    {
        int pid = fork();
        if (pid == 0)
        {
            exec(name);
        }
        else
        {
            set_dsid(pid, j);
            pid_arr[j] = pid;
        }
    }
    MonitorTraffic();
    for (int j = 1; j <= 3; ++j)
        waitpid(pid_arr[j], &time_usage[j]);
    printf("%s task time usage(ms) | ", name);
    for (int j = 1; j <= 3; ++j)
        printf("dsid %d: %d | ", j, time_usage[j]);
    putchar('\n');
    return;
}

int main()
{
    DsidParam dsid_param[] = {{0, 0, 0, 0xF}, {1e3, 0, 0x100, 0xF0}, {1e3, 0, 0x4, 0xF00}, {5e4, 0, 0x100, 0xF000}};

    for (int i = 0; i <= 3; ++i)
        set_dsid_param(i, dsid_param[i].bucket_freq, dsid_param[i].bucket_size,
                       dsid_param[i].bucket_inc, dsid_param[i].l2_mask);
    GenerateWorkload("sort");
    GenerateWorkload("integrate");
    puts("\ndsid demo 2 finished!\n");
    return 0;
}
