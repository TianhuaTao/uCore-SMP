#include "defs.h"
#include "proc.h"
#include "trap.h"
#include "timer.h"
#include "log.h"
#include "riscv.h"
struct proc pool[NPROC];
char kstack[NPROC][PAGE_SIZE];
__attribute__((aligned(4096))) char ustack[NPROC][PAGE_SIZE];
char trapframe[NPROC][PAGE_SIZE];

extern char boot_stack_top[];

const int64 BIGSTRIDE = 0x7FFFFFFFLL;

struct proc *curr_proc()
{
    return mycpu()->proc;
}
struct proc idle[NCPU];

void procinit(void)
{
    struct proc *p;
    for (p = pool; p < &pool[NPROC]; p++)
    {
        init_spin_lock(&p->lock);
        p->state = UNUSED;
        p->kstack = (uint64)kstack[p - pool];
        p->ustack = (uint64)ustack[p - pool];
        p->trapframe = (struct trapframe *)trapframe[p - pool];
    }
    for (int i = 0; i < NCPU; i++)
    {
        idle[i].kstack = (uint64)boot_stack_top;
        idle[i].pid = 0;
        idle[i].killed = 0;
    }
}

int allocpid()
{
    static int PID = 1;
    return PID++;
}

struct proc *allocproc(void)
{
    struct proc *p;
    for (p = pool; p < &pool[NPROC]; p++)
    {
        if (p->state == UNUSED)
        {
            goto found;
        }
    }
    return 0;

found:
    p->pid = allocpid();
    p->state = USED;
    memset(&p->context, 0, sizeof(p->context));
    memset(p->trapframe, 0, PAGE_SIZE);
    memset((void *)p->kstack, 0, PAGE_SIZE);
    p->context.ra = (uint64)usertrapret;
    p->context.sp = p->kstack + PAGE_SIZE;

    p->stride = 0;
    p->priority = 16;
    p->cpu_time = 0;
    p->last_start_time = 0;
    return p;
}
// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid()
{
    int id = r_tp();
    return id;
}

struct spinlock schd_lock;
void init_scheduler()
{
    init_spin_lock(&schd_lock);
}
void scheduler(void)
{

    for (;;)
    {
        uint64 min_stride = ~0ULL;
        struct proc *next_proc = NULL;
        // debugcore("acquiring schd_lock");
        acquire(&schd_lock);
        // debugcore("acquired schd_lock");
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
            // debugcore("acquiring next_proc %d" ,next_proc->pid);
            acquire(&next_proc->lock);
            // debugcore("acquired next_proc %d",next_proc->pid);
        }
        // debugcore("releasing schd_lock");
        release(&schd_lock);
        // debugcore("released schd_lock");

        if (next_proc != NULL)
        {
            struct cpu *mycore = mycpu();
            next_proc->state = RUNNING;
            mycore->proc = next_proc;
            next_proc->last_start_time = get_time_ms();
            uint64 pass = BIGSTRIDE / (next_proc->priority);
            // debugf("pass = %p, priority=%d\n", pass,  next_proc->priority);
            next_proc->stride += pass;
            // debugf("run pid=%d, stride=%p\n", next_proc->pid, next_proc->stride);
            // debugf("[cpu=%d] run pid=%d, entry=%p", cpuid(),next_proc->pid, next_proc->entry);
            swtch(&idle[cpuid()].context, &next_proc->context);
            // debugcore("pid=%d, locked=%d ,cpu=%d",next_proc->pid,next_proc->lock.locked, next_proc->lock.cpu->core_id)
            // debugcore("releasing next_proc");
            release(&next_proc->lock);
            // debugcore("released next_proc");
        }
        else
        {
            debugcore("no proc to run");
            break;
        }
    }
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

    // kill proc running for more than 5 seconds
    if (p->cpu_time > 5 * 1000)
    {
        infof("kill proc %d for running too long\n", p->pid);
        p->state = UNUSED;
        // exit(-1);
    }
    swtch(&p->context, &idle[cpuid()].context);
}

// Give up the CPU for one scheduling round.
void yield(void)
{
    mycpu()->proc->state = RUNNABLE;
    sched();
}

void exit(int code)
{
    struct proc *p = curr_proc();
    infof("proc %d exit with %d\n", p->pid, code);
    p->state = UNUSED;
    // finished();
    sched();
}