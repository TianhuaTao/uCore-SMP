#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <dsid.h>
#include <ucore.h>
#include <string.h>

#define MAX_TASK_NUM 5

int task_num;
char *task_name[MAX_TASK_NUM + 1];
int task_dsid[MAX_TASK_NUM + 1];

void MonitorTraffic(void)
{
    sleep(100);
    int64 prev_l2_traffic[MAX_TASK_NUM + 1];
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
            return;
        }
        else
        {
            set_dsid(pid, dsid);
            task_pid[i] = pid;
        }
    }
    MonitorTraffic();
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
    DsidParam dsid_param[] = {
        // for kernel and monitor
        {10000, 0x800, 40, 0x000F}, // 3125 0
        // no limit
        {100, 0x800, 100, 0xFFF0}, // inf   1

        // asymmetric
        // mem limited, cache isolated
        {7800, 0x800, 80, 0xFFC0}, // 8012  2
        {7800, 0x800, 10, 0x0030}, // 1001  3
        // mem limit, cache shared
        {7800, 0x800, 80, 0xFFF0}, // 8012  4
        {7800, 0x800, 10, 0xFFF0}, // 1001  5
        // mem inf, cache isolated
        {100, 0x800, 100, 0xFFC0}, //       6
        {100, 0x800, 100, 0x0030}, //       7

        // symmetric
        // mem limited, cache isolated
        {9300, 0x800, 60, 0x03F0}, //       8
        {9300, 0x800, 60, 0xFC00}, //       9
        // mem limit, cache shared
        {9300, 0x800, 60, 0xFFF0}, // 5040  10
        // mem inf, cache isolated
        {100, 0x800, 100, 0xFC00}, //       11
        {100, 0x800, 100, 0x03F0}, //       12

        // asymmetric
        // mem limited, cache isolated
        {7800, 0x800, 70, 0xFFC0}, // 7010  13
        {7800, 0x800, 20, 0x0030}, // 2003  14
        // mem limit, cache shared
        {7800, 0x800, 70, 0xFFF0}, // 7010  15
        {7800, 0x800, 20, 0xFFF0}, // 2003  16
    };

    int dsid_param_limit = sizeof(dsid_param) / sizeof(DsidParam);
    if (argc < 2 || argc % 2 == 0 || argc > 2 * MAX_TASK_NUM + 1)
    {
        puts("please input correct workload and dsid!");
        return -1;
    }

    task_num = (argc - 1) / 2;

    for (int i = 0; i < task_num; ++i)
    {
        task_name[i] = argv[i * 2 + 1];
        task_dsid[i] = i + 1;
        int dsid_param_id = atoi(argv[i * 2 + 2]);
        set_dsid_param(task_dsid[i],
                       dsid_param[dsid_param_id].bucket_freq,
                       dsid_param[dsid_param_id].bucket_size,
                       dsid_param[dsid_param_id].bucket_inc,
                       dsid_param[dsid_param_id].l2_mask);
        if (dsid_param_id > dsid_param_limit || dsid_param_id < 0)
        {
            printf("workload %s dsid %d out of range!", task_name[i], dsid_param_id);
            return -2;
        }
    }

    task_dsid[task_num] = 0;
    task_name[task_num] = "others";
    set_dsid_param(0, dsid_param[0].bucket_freq, dsid_param[0].bucket_size,
                   dsid_param[0].bucket_inc, dsid_param[0].l2_mask);
    GenerateWorkload();
    puts("dsid demo finished!");
    return 0;
}
