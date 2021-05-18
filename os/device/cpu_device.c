#include <arch/cpu.h>
#include <file/file.h>

struct cpu_stat
{
    uint64 uptime;
    uint64 sample_duration;
    uint64 sample_busy_duration;
};

int64 cpu_write(char *src, int64 len, int from_user)
{
    // you cannot write to cpu stat
    return -1;
}
int64 cpu_read(char *dst, int64 len, int to_user)
{
    struct cpu_stat stat_buf[NCPU];

    if (len > NCPU * sizeof(struct cpu_stat))
    {
        len = NCPU * sizeof(struct cpu_stat);
    }

    for (int i = 0; i < NCPU; i++)
    {
        uint64 busy_average = 0;
        uint64 all_average = 0;
            // printf("%d: ", i);

        for (int j = 0; j < SAMPLE_SLOT_COUNT; j++)
        {
            // printf("%l ", cpus[i].busy_time[j]);
            busy_average += cpus[i].busy_time[j];
            all_average += cpus[i].sample_duration[j];
        }
            // printf("\n");

        all_average /= SAMPLE_SLOT_COUNT;
        busy_average /= SAMPLE_SLOT_COUNT;
        // printf("busy_average=%l\n", busy_average);
        stat_buf[i].sample_duration = all_average;
        stat_buf[i].sample_busy_duration = busy_average;
        stat_buf[i].uptime = r_time()-cpus[i].start_cycle;

    }

    if (either_copyout(dst, stat_buf, len, to_user) < 0)
        return -1;

    return len;
}

void cpu_device_init()
{
    device_rw_handler[CPU_DEVICE].read = cpu_read;
    device_rw_handler[CPU_DEVICE].write = cpu_write;
}
