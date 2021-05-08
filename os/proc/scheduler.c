#include "scheduler.h"
#include <proc/proc.h>
#include <ucore/ucore.h>

void init_scheduler() {
}
void scheduler(void) {
    for (;;) {
        uint64 min_stride = ~0ULL;
        struct proc *next_proc = NULL;
        int any_proc = FALSE;
        // lock when picking proc
        acquire(&pool_lock);

        for (struct proc *p = pool; p < &pool[NPROC]; p++) {
            if (!p->state == UNUSED) {
                any_proc = TRUE;
                // debugcore("state=%d", p->state);
            }
            if (p->state == RUNNABLE && !p->lock.locked) {
                if (p->stride < min_stride) {
                    min_stride = p->stride;
                    next_proc = p;
                }
            }
        }

        if (next_proc != NULL) {
            // found a process to run, lock down the proc;
            acquire(&next_proc->lock);
        }
        // picking proc done
        release(&pool_lock);

        if (next_proc != NULL) {

            struct cpu *mycore = mycpu();
            mycore->proc = next_proc;
            next_proc->state = RUNNING;

            next_proc->last_start_time = get_time_ms();
            uint64 pass = BIGSTRIDE / (next_proc->priority);
            next_proc->stride += pass;
            // print_proc(next_proc);
            // debugcore("in scheduler before swtch");
            // print_cpu(mycpu());

            swtch(&mycpu()->context, &next_proc->context);
            // debugcore("in scheduler after swtch");
            // print_cpu(mycpu());
            mycore->proc = NULL;
            release(&next_proc->lock);
        } else {
            if (!any_proc) {
                debugcore("zero proc in pool");
                break;
            }
            // debugcore("no proc to run intr=%d", intr_get());
            // break;
        }
    }
}