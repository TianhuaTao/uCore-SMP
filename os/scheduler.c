#include "scheduler.h"
#include "ucore.h"
#include "proc.h"

void init_scheduler()
{
}
void scheduler(void)
{
    for (;;)
    {
        uint64 min_stride = ~0ULL;
        struct proc *next_proc = NULL;

        // lock when picking proc
        acquire(&pool_lock);

        for (struct proc *p = pool; p < &pool[NPROC]; p++)
        {
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

            struct cpu *mycore = mycpu();
            mycore->proc = next_proc;
            next_proc->state = RUNNING;



            next_proc->last_start_time = get_time_ms();
            uint64 pass = BIGSTRIDE / (next_proc->priority);
            next_proc->stride += pass;

            swtch(&mycpu()->context, &next_proc->context);

            release(&next_proc->lock);
        }
        else
        {
            // debugcore("no proc to run");
            // break;
        }
    }
}