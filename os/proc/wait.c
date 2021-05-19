#include <proc/proc.h>
/**
 * wait for child process with pid to exit
 */
int wait(int pid, int *wstatus_va)
{
    struct proc *p = curr_proc();
    struct proc *maybe_child;

    acquire(&wait_lock);

    for (;;)
    {

        bool havekids = FALSE;
        for (maybe_child = pool; maybe_child < &pool[NPROC]; maybe_child++)
        {
            if (maybe_child->parent == p)
            {

                acquire(&maybe_child->lock);

                if (pid < 0 || maybe_child->pid == pid) // this is one of the target
                {
                    havekids = TRUE;
                    if (maybe_child->state == ZOMBIE)
                    {
                        // Found one.
                        int child_pid = maybe_child->pid;
                        int wstatus = maybe_child->exit_code;
                        if (wstatus_va && copyout(p->pagetable, (uint64)wstatus_va, (char *)&wstatus,  sizeof(int)) < 0)
                        {
                            release(&maybe_child->lock);
                            release(&wait_lock);
                            return -1;
                        }
                        freeproc(maybe_child);
                        release(&maybe_child->lock);
                        release(&wait_lock);
                        return child_pid;
                    }
                }
                release(&maybe_child->lock);
            }
        }

        if (!havekids || p->killed)
        {
            release(&wait_lock);
            return -1;
        }

        // Wait for a child to exit.
        sleep(p, &wait_lock);
    }
}
