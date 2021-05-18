#include <proc/proc.h>

// Give up the CPU for one scheduling round.
void yield(void) {
    struct proc *p = curr_proc();
    KERNEL_ASSERT(p != NULL, "yield() has no current proc");
    acquire(&p->lock);
    p->state = RUNNABLE;
    switch_to_scheduler();
    release(&p->lock);
}
