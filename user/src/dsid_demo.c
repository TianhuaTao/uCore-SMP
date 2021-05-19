#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <dsid.h>
#include <ucore.h>
#include <string.h>

#define MAX_DSID 4
#define MAX_TASK_NUM 3

int task_num;
char *task_name[MAX_TASK_NUM + 1];
int task_dsid[MAX_TASK_NUM + 1];

void MonitorTraffic(void)
{
    sleep(100);
    int64 prev_l2_traffic[4];
    for (int i = 0; i <= task_num; ++i)
        prev_l2_traffic[task_dsid[i]] = get_l2_traffic(task_dsid[i]);
    puts("L1 to L2 traffic speed(KB/s): ");
    for (int i = 0; i < 10; ++i)
    {
        for (int i = 0; i <= task_num; ++i)
        {
            int dsid = task_dsid[i];
            int64 l2_traffic = get_l2_traffic(dsid);
            if (l2_traffic < prev_l2_traffic[dsid])
                prev_l2_traffic[dsid] -= UINT_MAX;
            printf("%s, dsid %d: %d | ", task_name[i], dsid, (l2_traffic - prev_l2_traffic[dsid]) * 8 / 1024);
            prev_l2_traffic[dsid] = l2_traffic;
        }
        putchar('\n');
        sleep(1000);
    }
    return;
}

void GenerateWorkload(void)
{
    sleep(50);
    puts("generating workload:");
    for (int i = 0; i < task_num; ++i)
        printf("task: %s, dsid: %d | ", task_name[i], task_dsid[i]);
    putchar('\n');
    sleep(50);

    int task_pid[MAX_TASK_NUM + 1], time_usage[MAX_TASK_NUM + 1];

    for (int i = 0; i < task_num; ++i)
    {
        int pid = fork(), dsid = task_dsid[i];
        if (pid == 0)
        {
            sched_yield();
            exec(task_name[i]);
        }
        else
        {
            set_dsid(pid, dsid);
            task_pid[i] = pid;
        }
    }
    // MonitorTraffic();
    for (int i = 0; i < task_num; ++i)
        waitpid(task_pid[i], &time_usage[i]);
    puts("time usage:");
    for (int i = 0; i < task_num; ++i)
        printf("%s, pid %d, dsid %d: %dms || ", task_name[i], task_pid[i], task_dsid[i], time_usage[i]);
    putchar('\n');
}

typedef struct
{
    uint32 bucket_freq, bucket_size, bucket_inc, l2_mask;
} DsidParam;

int main(int argc, char *argv[])
{
    DsidParam dsid_param[MAX_DSID + 1] = {
        {780, 0x800, 5, 0xF},      // 5008
        {1e2, 0x800, 1e2, 0xFFF0}, // inf
        {970, 0x800, 10, 0xF0},    // 8054
        {780, 0x800, 4, 0xF00},    // 4006
        {1e3, 0x800, 5, 0xF000},   // 3906
    };

    for (int i = 0; i <= MAX_DSID; ++i)
        set_dsid_param(i, dsid_param[i].bucket_freq, dsid_param[i].bucket_size,
                       dsid_param[i].bucket_inc, dsid_param[i].l2_mask);
    if (argc < 2 || argc % 2 == 0 || argc > 2 * MAX_TASK_NUM + 1)
    {
        puts("please input correct workload and dsid!");
        return -1;
    }

    task_num = (argc - 1) / 2;

    for (int i = 0; i < task_num; ++i)
    {
        task_name[i] = argv[i * 2 + 1];
        task_dsid[i] = atoi(argv[i * 2 + 2]);
        task_dsid[i] = argv[i * 2 + 2][0] - '0';
        if (task_dsid[i] > MAX_DSID || task_dsid[i] < 0)
        {
            printf("workload %s dsid %d out of range!", task_name[i], task_dsid[i]);
            return -2;
        }
    }

    task_dsid[task_num] = 0;
    task_name[task_num] = "others";
    GenerateWorkload();
    puts("dsid demo finished!");
    return 0;
}
