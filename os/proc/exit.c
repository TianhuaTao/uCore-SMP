#include <proc/proc.h>

static void close_proc_files(struct proc *p)
{
    // close files
    for (int i = 0; i < FD_MAX; ++i)
    {
        if (p->files[i] != NULL)
        {
            fileclose(p->files[i]);
            p->files[i] = NULL;
        }
    }
}

/**
 * @brief make p->parent = NULL
 * 
 * If p is zombie, just free the struct
 * don't need to free the memory or files because they should be freed before p
 * became a zombie
 * 
 * @param p 
 */
void reparent(struct proc *p)
{
    tracecore("reparent");
    KERNEL_ASSERT(holding(&wait_lock), "reparent lock");

    if (p->state == ZOMBIE)
    {
        freeproc(p);
    }
    else
    {
        p->parent = NULL;
    }
}

/**
 * Exit current running process
 * will do:
 * 1. close files and dir
 * 2. reparent this process's children
 * 3. free all the memory and pagetables
 * 4. set the state to ZOMBIE if parent is alive 
 *    or just free this proc struct if parent is dead
 */
void exit(int code)
{   
    tracecore("exit");
    struct proc *p = curr_proc();
    int pid_tmp = p->pid;   // keep for infof
    (void) pid_tmp;
    acquire(&p->lock);
    p->exit_code = code;

    // 1. close files
    close_proc_files(p);

    iput(p->cwd);
    p->cwd = NULL;
    release(&p->lock);

    // 2. reparent this process's children
    acquire(&wait_lock);
    struct proc *child;
    for (child = pool; child < &pool[NPROC]; child++)
    {
        if (child != p) // avoid deadlock
        {
            // print_proc(child);
            if (child->state != UNUSED && child->parent == p)
            {
                // found a child
                acquire(&child->lock);
                reparent(child);
                release(&child->lock);

            }
        }
    }
    release(&wait_lock);

    // 3. free all the memory and pagetables
    KERNEL_ASSERT(p->trapframe !=NULL, "");
    KERNEL_ASSERT(p->pagetable !=NULL, "");

    proc_free_mem_and_pagetable(p);

    // 4. set the state
    acquire(&wait_lock);
    acquire(&p->lock);
    if (p->parent != NULL)
    {
        p->state = ZOMBIE;
        wakeup(p->parent);
    }
    else
    {
        // parent is dead
        freeproc(p);
    }
    release(&wait_lock);

    infof("proc %d exit with %d\n", pid_tmp, code);
    switch_to_scheduler();
}
