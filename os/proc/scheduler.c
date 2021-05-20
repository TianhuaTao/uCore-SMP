#include "scheduler.h"
#include <proc/proc.h>
#include <ucore/ucore.h>
#include <arch/timer.h>
void init_scheduler()
{
}

void scheduler(void)
{
    uint64 busy = 0;
    uint64 all = 0;
    uint64 timestamp1 = r_cycle();
    for (;;)
    {
        uint64 min_stride = ~0ULL;
        struct proc *next_proc = NULL;
        int any_proc = FALSE;
        // lock when picking proc
        acquire(&pool_lock);

        for (struct proc *p = pool; p < &pool[NPROC]; p++)
        {
            if (!p->state == UNUSED)
            {
                any_proc = TRUE;
                // debugcore("state=%d", p->state);
            }
            if (p->state == RUNNABLE && !p->lock.locked)
            {
                if (p->stride < min_stride)
                {
                    min_stride = p->stride;
                    next_proc = p;
                }
            }
        }

        if (next_proc != NULL)
        {
            // found a process to run, lock down the proc;
            acquire(&next_proc->lock);
        }
        // picking proc done
        release(&pool_lock);

        if (next_proc != NULL)
        {
                // printf("Core %d pick proc %s\n", cpuid(),next_proc->name);

            struct cpu *mycore = mycpu();
            mycore->proc = next_proc;
            next_proc->state = RUNNING;
            start_timer_interrupt();

            uint64 busy_start = r_cycle();
            next_proc->last_start_time = get_time_ms();
            uint64 pass = BIGSTRIDE / (next_proc->priority);
            next_proc->stride += pass;
            w_dsid(next_proc->dsid);
            if (is_panic_addr(next_proc->context.ra))
                debugcore("1070 in scheduler !\n");
            mmiowb();
            swtch(&mycpu()->context, &next_proc->context);
            mmiowb();

            busy += r_cycle() - busy_start;
            uint64 time_delta = get_time_ms() - next_proc->last_start_time;
            next_proc->cpu_time += time_delta;

            stop_timer_interrupt();
            mycore->proc = NULL;

            release(&next_proc->lock);
        }
        else
        {
            if (!any_proc)
            {
                debugcore("zero proc in pool");
                break;
                // end scheduler, kernel will shutdown
            }
        }
        // printf("core%d\n",cpuid());
        // sample cpu usage
        uint64 now = r_cycle();
        all += now - timestamp1;
        timestamp1 = now;
        // sample rate 10 Hz
        if (all > (MS_TO_CYCLE(100)))
        {
            struct cpu *core = mycpu();

            // record one sample
            int next_slot = core->next_slot;
            core->sample_duration[next_slot] = all;
            core->busy_time[next_slot] = busy;
            core->next_slot = (next_slot + 1) % SAMPLE_SLOT_COUNT;
            // printf("busy/all = %d/%d\n", (int)busy, (int)all);
            all = 0;
            busy = 0;
        }
    }
}