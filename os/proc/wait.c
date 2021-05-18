#include <proc/proc.h>
/**
 * wait for child process with pid to exit
 */
int wait(int pid, int *code) {
    // TODO: use sleep
    struct proc *np;
    int havekids;
    struct proc *p = curr_proc();
    acquire(&p->lock);
    for (;;) {
        // Scan through table looking for exited children.
        havekids = FALSE;
        for (np = pool; np < &pool[NPROC]; np++) {
            // info("pid=%d, np->pid=%d, p=%d, parent=%d,\n",pid,np->pid,p, np->parent);
            if (np->parent == p) {
                KERNEL_ASSERT(np->state != UNUSED, "");
                acquire(&np->lock);
                if (pid <= 0 || np->pid == pid) {
                    havekids = TRUE;
                    if (np->state == ZOMBIE) {
                        // Found one.
                        np->state = UNUSED;
                        pid = np->pid;
                        *code = np->exit_code;
                        freeproc(np);
                        release(&np->lock);
                        release(&p->lock);
                        return pid;
                    }
                }
                release(&np->lock);
            }
        }
        if (!havekids) {
            debugcore("no kids\n");
            release(&p->lock);
            return -1;
        }

        // debugf("pid %d keep waiting", p->pid);
        p->state = RUNNABLE;
        switch_to_scheduler();
    }
}
