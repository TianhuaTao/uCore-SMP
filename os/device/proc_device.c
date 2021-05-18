#include <arch/cpu.h>
#include <file/file.h>
#include <ucore/ucore.h>
#include <proc/proc.h>
uint64 get_free_page_count();

struct proc_stat
{
    char name[PROC_NAME_MAX];
    int pid;
    int ppid; // Parent process
    int state;
    uint64 heap_sz;
    uint64 total_size;
    uint64 cpu_time; // ms, user and kernel
};

int64 proc_write(char *src, int64 len, int from_user)
{
    // you cannot write to mem stat
    return -1;
}
int64 proc_read(char *dst, int64 len, int to_user)
{
    struct proc_stat stat_buf[100];
    printf("len %d\n",len);

    if (len > 100 * sizeof(struct proc_stat))
    {
        return -1;
    }

    int cnt = 0;
    acquire(&pool_lock);
    for (struct proc *p = pool; p < &pool[NPROC]; p++)
    {
        if ((cnt + 1) * sizeof(struct proc_stat) > len)
            break;
        if (p->state != UNUSED)
        {
            acquire(&p->lock);
            strncpy(stat_buf[cnt].name, p->name, PROC_NAME_MAX);
            stat_buf[cnt].pid = p->pid;
            if (p->parent)
            {
                stat_buf[cnt].ppid = p->parent->pid;
            }
            else
            {
                stat_buf[cnt].ppid = -1;
            }
            stat_buf[cnt].heap_sz = p->heap_sz;
            stat_buf[cnt].total_size = p->total_size;
            stat_buf[cnt].cpu_time = p->cpu_time;
            stat_buf[cnt].state = p->state;

            release(&p->lock);
            cnt++;
        }
    }
    printf("cnt %d\n",cnt);
    release(&pool_lock);

    if (either_copyout(dst, stat_buf, cnt * sizeof(struct proc_stat), to_user) < 0)
    {
        return -1;
    }

    return cnt * sizeof(struct proc_stat);
}

void proc_device_init()
{
    device_rw_handler[PROC_DEVICE].read = proc_read;
    device_rw_handler[PROC_DEVICE].write = proc_write;
}
