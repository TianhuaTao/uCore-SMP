#include <proc/proc.h>
#include <trap/trap.h>

/**
 * @brief fork current process
 * 
 * @return int 0 or child pid, -1 on error
 */
int fork() {
    int i, pid;
    struct proc *np;
    struct proc *p = curr_proc();

    // Allocate process.
    if ((np = allocproc()) == 0) {
        return -1;
    }

    // Copy user memory from parent to child.
    if (uvmcopy(p->pagetable, np->pagetable, p->total_size) < 0) {
        freeproc(np);
        release(&np->lock);
        return -1;
    }
    np->total_size = p->total_size;
    np->stride  = p->stride;
    // copy saved user registers.
    *(np->trapframe) = *(p->trapframe);

    // Cause fork to return 0 in the child.
    np->trapframe->a0 = 0;

    // increment reference counts on open file descriptors.
    for (i = 0; i < FD_MAX; i++)
        if (p->files[i])
            np->files[i] = filedup(p->files[i]);
    np->cwd = idup(p->cwd);

    safestrcpy(np->name, p->name, sizeof(p->name));

    pid = np->pid;

    release(&np->lock);

    acquire(&wait_lock);
    np->parent = p;
    release(&wait_lock);

    acquire(&np->lock);
    np->state = RUNNABLE;
    release(&np->lock);

    return pid;
}
