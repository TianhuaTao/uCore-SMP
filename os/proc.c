#include "defs.h"
#include "proc.h"
#include "trap.h"
#include "timer.h"
#include "log.h"
#include "riscv.h"
#include "file.h"
#include "memory_layout.h"
struct proc pool[NPROC];

__attribute__((aligned(16))) char kstack[NPROC][KSTACK_SIZE];
extern char trampoline[];

extern char boot_stack_top[];
extern char boot_stack[];
struct spinlock pool_lock;
struct
{
    int pid;    // the next available pid
    struct spinlock lock;
} next_pid;
struct proc *curr_proc()
{
    return mycpu()->proc;
}

struct proc *findproc(int pid)
{
    struct proc *p = NULL;
    acquire(&pool_lock);
    for (p = pool; p < &pool[NPROC]; p++)
    {
        if (p->state != UNUSED && p->pid == pid)
        {
            break;
        }
    }
    release(&pool_lock);
    return p;
}

void procinit(void)
{
    struct proc *p;
    init_spin_lock_with_name(&pool_lock, "pool_lock");
    for (p = pool; p < &pool[NPROC]; p++)
    {
        init_spin_lock(&p->lock);
        p->state = UNUSED;
        p->waiting_target = NULL;
        p->kstack = (uint64)kstack[p - pool];
        // must after kinit()
        p->trapframe = alloc_physical_page();
    }

    next_pid.pid = 1;
    init_spin_lock_with_name(&next_pid.lock, "next_pid.lock");
}



int alloc_pid()
{
    acquire(&next_pid.lock);
    int pid = next_pid.pid;
    next_pid.pid++;
    release(&next_pid.lock);
    return pid;
}
pagetable_t
proc_pagetable(struct proc *p)
{
    pagetable_t pagetable;

    // An empty page table.
    pagetable = create_empty_user_pagetable();
    if (pagetable == NULL)
        panic("cannot create empty user pagetable");

    if (mappages(pagetable, TRAMPOLINE, PGSIZE,
                 (uint64)trampoline, PTE_R | PTE_X) < 0)
    {
        free_user_mem_and_pagetables(pagetable, 0);
        return 0;
    }

    if ((p->trapframe = (struct trapframe *)alloc_physical_page()) == 0)
    {
        panic("alloc trapframe page failed\n");
    }

    // map the trapframe just below TRAMPOLINE, for trampoline.S.
    if (mappages(pagetable, TRAPFRAME, PGSIZE,
                 (uint64)(p->trapframe), PTE_R | PTE_W) < 0)
    {
        ;
        panic("Can not map TRAPFRAME");
    }

    return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
    uvmunmap(pagetable, TRAMPOLINE, 1, FALSE);
    uvmunmap(pagetable, TRAPFRAME, 1, FALSE);
    free_user_mem_and_pagetables(pagetable, sz);
}

static void
freeproc(struct proc *p)
{
    if (p->trapframe)
        recycle_physical_page((void *)p->trapframe);
    p->trapframe = NULL;
    if (p->pagetable)
        proc_freepagetable(p->pagetable, p->total_size);
    p->pagetable = NULL;
    p->state = UNUSED;

    // close files
    for (int i = 0; i < FD_MAX; ++i)
    {
        if (p->files[i] != NULL)
        {
            fileclose(p->files[i]);
            p->files[i] = 0;
        }
    }
}

struct proc *allocproc(void)
{
    struct proc *p;
    acquire(&pool_lock);
    for (p = pool; p < &pool[NPROC]; p++)
    {
        if (p->state == UNUSED)
        {
            acquire(&p->lock);
            release(&pool_lock);
            goto found;
        }
    }
    release(&pool_lock);
    debugf("no proc alloced");
    return 0;

found:
    p->pid = alloc_pid();
    p->state = USED;
    p->total_size = 0;
    p->heap_sz = 0;
    p->exit_code = -1;
    p->parent = NULL;
    p->ustack_bottom = 0;
    p->pagetable = proc_pagetable(p);
    if (p->pagetable == 0)
    {
        panic("");
    }
    memset(&p->context, 0, sizeof(p->context));
    memset((void *)p->kstack, 0, KSTACK_SIZE);
    // debugf("memset done");
    p->context.ra = (uint64)usertrapret; // used in swtch()
    p->context.sp = p->kstack + KSTACK_SIZE;

    p->stride = 0;
    p->priority = 16;
    p->cpu_time = 0;
    p->last_start_time = 0;
    if (init_mailbox(&p->mb) == 0)
    {
        panic("init mailbox failed");
    }
    // debugf("before return");
    return p;
}


// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
    struct proc *p = curr_proc();
    if (p->state == RUNNING)
        panic("sched running");
    uint64 runtime = get_time_ms() - p->last_start_time;
    p->cpu_time += runtime;
    // debugf("cputime, last runtime = %p, %p", p->cpu_time, runtime);

    swtch(&p->context, &mycpu()->context);
}

// Give up the CPU for one scheduling round.
void yield(void)
{
    mycpu()->proc->state = RUNNABLE;
    sched();
}

int fork(void)
{
    int pid;
    struct proc *np;
    struct proc *p = curr_proc();
    debugf("inside fork");

    // Allocate process.
    if ((np = allocproc()) == 0)
    {
        panic("allocproc\n");
    }
    // debugf("inside fork allocproc done");

    // Copy user memory from parent to child.
    if (uvmcopy(p->pagetable, np->pagetable, p->total_size) < 0)
    {
        release(&np->lock);
        panic("uvmcopy\n");
    }
    np->total_size = p->total_size;

    // copy saved user registers.
    *(np->trapframe) = *(p->trapframe);

    for (int i = 0; i < FD_MAX; ++i)
        if (p->files[i] != 0 && p->files[i]->type != FD_NONE)
        {
            p->files[i]->ref++;
            np->files[i] = p->files[i];
        }
    // Cause fork to return 0 in the child.
    np->trapframe->a0 = 0;
    pid = np->pid;

    np->parent = p;

    np->state = RUNNABLE;
    release(&np->lock);
    return pid;
}

int exec(char *name)
{
    int id = get_id_by_name(name);
    if (id < 0)
        return -1;
    struct proc *p = curr_proc();
    proc_freepagetable(p->pagetable, p->total_size);
    p->total_size = 0;
    p->pagetable = proc_pagetable(p);
    if (p->pagetable == 0)
    {
        panic("");
    }
    loader(id, p);
    return 0;
}

int spawn(char *filename)
{
    int pid;
    struct proc *np;
    struct proc *p = curr_proc();

    // Allocate process.
    if ((np = allocproc()) == 0)
    {
        panic("allocproc\n");
    }
    // info("alloc\n");
    // Copy user memory from parent to child.
    if (uvmcopy(p->pagetable, np->pagetable, p->total_size) < 0)
    {
        panic("uvmcopy\n");
    }
    np->total_size = p->total_size;

    // copy saved user registers.
    *(np->trapframe) = *(p->trapframe);

    // Cause fork to return 0 in the child.
    np->trapframe->a0 = 0;
    pid = np->pid;
    np->parent = p;
    np->state = RUNNABLE;

    // info("fork done\n");

    char name[200];
    copyinstr(p->pagetable, name, (uint64)filename, 200);
    infof("sys_exec %s\n", name);

    int id = get_id_by_name(name);
    // info("id=%d\n", id);
    if (id < 0)
        return -1;
    // info("free\n");
    proc_freepagetable(np->pagetable, np->total_size);
    np->total_size = 0;
    np->pagetable = proc_pagetable(np);
    if (np->pagetable == 0)
    {
        panic("");
    }
    // info("load\n");
    loader(id, np);
    release(&np->lock);
    return pid;
}

/**
 * wait for child process with pid to exit
 */
int wait(int pid, int *code)
{
    struct proc *np;
    int havekids;
    struct proc *p = curr_proc();

    for (;;)
    {
        // Scan through table looking for exited children.
        havekids = FALSE;
        for (np = pool; np < &pool[NPROC]; np++)
        {
            // info("pid=%d, np->pid=%d, p=%d, parent=%d,\n",pid,np->pid,p, np->parent);
            if (np->state != UNUSED && np->parent == p && (pid <= 0 || np->pid == pid))
            {
                havekids = TRUE;
                if (np->state == ZOMBIE)
                {
                    // Found one.
                    np->state = UNUSED;
                    pid = np->pid;
                    *code = np->exit_code;
                    return pid;
                }
            }
        }
        if (!havekids)
        {
            infof("no kids\n");
            return -1;
        }

        // debugf("pid %d keep waiting", p->pid);
        p->state = RUNNABLE;
        sched();
    }
}

/**
 * Exit current running process
 */
void exit(int code)
{
    struct proc *p = curr_proc();
    p->exit_code = code;
    infof("proc %d exit with %d\n", p->pid, code);
    freeproc(p);
    if (p->parent != 0)
    {
        tracef("wait for parent to clean\n");
        p->state = ZOMBIE;
    }
    debugf("before sched");
    sched();
}

/**
 * Allocate a file descriptor of this process for the given file
 */
int fdalloc(struct file *f)
{
    struct proc *p = curr_proc();
    // fd = 0,1,2 is reserved for stdio/stdout
    for (int i = 3; i < FD_MAX; ++i)
    {
        if (p->files[i] == 0)
        {
            p->files[i] = f;
            return i;
        }
    }
    return -1;
}

void wakeup(void *waiting_target) {
    struct proc *p;

    for (p = pool; p < &pool[NPROC]; p++) {
        if (p != curr_proc()) {
            acquire(&p->lock);
            if (p->state == SLEEPING && p->waiting_target == waiting_target) {
                p->state = RUNNABLE;
            }
            release(&p->lock);
        }
    }
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *waiting_target, struct spinlock *lk) {
    struct proc *p = curr_proc();

    // Must acquire p->lock in order to
    // change p->state and then call sched.
    // Once we hold p->lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup locks p->lock),
    // so it's okay to release lk.

    acquire(&p->lock); //DOC: sleeplock1
    release(lk);

    // Go to sleep.
    p->waiting_target = waiting_target;
    p->state = SLEEPING;

    sched();

    // Tidy up.
    p->waiting_target = NULL;

    // Reacquire original lock.
    release(&p->lock);
    acquire(lk);
}
