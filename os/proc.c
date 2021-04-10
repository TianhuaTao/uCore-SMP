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
const int64 BIGSTRIDE = 0x7FFFFFFFLL;
struct spinlock pool_lock;

struct proc *curr_proc()
{
    return mycpu()->proc;
}
// struct proc idle[NCPU];

struct proc* findproc(int pid)
{
    struct proc *p = NULL;
    acquire(&pool_lock);
    for(p = pool; p < &pool[NPROC]; p++) {
        if(p->state != UNUSED && p->pid == pid) {
            break;
        }
    }
    release(&pool_lock);
    return p;
}


void procinit(void)
{
    struct proc *p;
    init_spin_lock(&pool_lock);
    for (p = pool; p < &pool[NPROC]; p++)
    {
        init_spin_lock(&p->lock);
        p->state = UNUSED;
        p->kstack = (uint64)kstack[p - pool];
    }
    // for (int i = 0; i < NCPU; i++)
    // {
    //     idle[i].kstack = (uint64)boot_stack+ i* 4*1024;
    //     idle[i].pid = i;
    //     // idle[i].killed = 0;
    // }
}

int allocpid()
{
    // TODO: add lock
    static int PID = 1;
    return PID++;
}

pagetable_t
proc_pagetable(struct proc *p)
{
    pagetable_t pagetable;

    // An empty page table.
    pagetable = uvmcreate();
    if(pagetable == 0)
        panic("");

    if(mappages(pagetable, TRAMPOLINE, PGSIZE,
                (uint64)trampoline, PTE_R | PTE_X) < 0){
        uvmfree(pagetable, 0);
        return 0;
    }

    if((p->trapframe = (struct trapframe *)kalloc()) == 0){
        panic("kalloc\n");
    }
    // map the trapframe just below TRAMPOLINE, for trampoline.S.
    if(mappages(pagetable, TRAPFRAME, PGSIZE,
                (uint64)(p->trapframe), PTE_R | PTE_W) < 0){;
        panic("");
    }

    return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmunmap(pagetable, TRAPFRAME, 1, 0);
    uvmfree(pagetable, sz);
}

static void
freeproc(struct proc *p)
{
    if(p->trapframe)
        kfree((void*)p->trapframe);
    p->trapframe = 0;
    if(p->pagetable)
        proc_freepagetable(p->pagetable, p->sz);
    p->pagetable = 0;
    p->state = UNUSED;
    for(int i = 0; i < FD_MAX; ++i) {
        if(p->files[i] != 0) {
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
    p->pid = allocpid();
    p->state = USED;
    p->sz = 0;
    p->heap_sz = 0;
    p->exit_code = -1;
    p->parent = 0;
    p->ustack = 0;
    p->pagetable = proc_pagetable(p);
    if(p->pagetable == 0){
        panic("");
    }
    memset(&p->context, 0, sizeof(p->context));
    memset((void *)p->kstack, 0, KSTACK_SIZE);
    // debugf("memset done");
    p->context.ra = (uint64)usertrapret;    // used in swtch()
        p->context.sp = p->kstack + KSTACK_SIZE;


    p->stride = 0;
    p->priority = 16;
    p->cpu_time = 0;
    p->last_start_time = 0;
    if(init_mailbox(&p->mb)==0){
        panic("init mailbox failed");
    } 
    // debugf("before return");
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

void init_scheduler()
{
    // init_spin_lock(&pool_lock);
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

            // intr_on();
            // asm volatile("mv t4, t4");
            // intr_off();

            // printf_k("go swtch to pid %d\n", next_proc->pid);

            // intr_on();
            // asm volatile("mv t5, t5");
            // intr_off();

            swtch(&mycpu()->context, &next_proc->context);
            // printf_k("intr_on=%d\n", intr_get());
            // printf_k("come back from pid %d\n", curr_proc()->pid);

            // switch back to idle here, RUNNING in IDEL process
            // uint64 sp = r_sp();
            // uint64 magic_num = 0x80213f58;
            // uint64* err_addr = (uint64*)magic_num;
            // printf_k("err_addr=%p\n",err_addr);
            // printf_k("before *(err_addr)=%p\n",*err_addr);
            // *err_addr = 0x100;
            // printf_k("after *(err_addr)=%p\n",*err_addr);
            release(&next_proc->lock);
        }
        else
        {
            // debugcore("no proc to run");
            // break;
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

    // // kill proc running for more than 5 seconds
    // if (p->cpu_time > 5 * 1000)
    // {
    //     infof("kill proc %d for running too long\n", p->pid);
    //     p->state = UNUSED;
    //     // exit(-1);
    // }
    // infof("before swtch");
    swtch(&p->context, &mycpu()->context);
}

// Give up the CPU for one scheduling round.
void yield(void)
{
    mycpu()->proc->state = RUNNABLE;
    sched();
}
int
fork(void)
{
    int pid;
    struct proc *np;
    struct proc *p = curr_proc();
    // debugf("inside fork");

    // Allocate process.
    if((np = allocproc()) == 0){
        panic("allocproc\n");
    }
    // debugf("inside fork allocproc done");
    
    // Copy user memory from parent to child.
    if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
        release(&np->lock);
        panic("uvmcopy\n");
    }
    np->sz = p->sz;

    // copy saved user registers.
    *(np->trapframe) = *(p->trapframe);

    for(int i = 0; i < FD_MAX; ++i)
        if(p->files[i] != 0 && p->files[i]->type != FD_NONE) {
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

int exec(char* name) {
    int id = get_id_by_name(name);
    if(id < 0)
        return -1;
    struct proc *p = curr_proc();
    proc_freepagetable(p->pagetable, p->sz);
    p->sz = 0;
    p->pagetable = proc_pagetable(p);
    if(p->pagetable == 0){
        panic("");
    }
    loader(id, p);
    return 0;
}

int spawn(char* filename){
    int pid;
    struct proc *np;
    struct proc *p = curr_proc();

    // Allocate process.
    if((np = allocproc()) == 0){
        panic("allocproc\n");
    }
    // info("alloc\n");
    // Copy user memory from parent to child.
    if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
        panic("uvmcopy\n");
    }
    np->sz = p->sz;

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
    if(id < 0)
        return -1;
    // info("free\n");
    proc_freepagetable(np->pagetable, np->sz);
    np->sz = 0;
    np->pagetable = proc_pagetable(np);
    if(np->pagetable == 0){
        panic("");
    }
    // info("load\n");
    loader(id, np);
    release(&np->lock);
    return pid;
}

int
wait(int pid, int* code)
{
    struct proc *np;
    int havekids;
    struct proc *p = curr_proc();

    for(;;){
        // Scan through table looking for exited children.
        havekids = 0;
        for(np = pool; np < &pool[NPROC]; np++){
            // info("pid=%d, np->pid=%d, p=%d, parent=%d,\n",pid,np->pid,p, np->parent);
            if(np->state != UNUSED && np->parent == p && (pid <= 0 || np->pid == pid)){
                havekids = 1;
                if(np->state == ZOMBIE){
                    // Found one.
                    np->state = UNUSED;
                    pid = np->pid;
                    *code = np->exit_code;
                    return pid;
                }
            }
        }
        if(!havekids){
            infof("no kids\n");
            return -1;
        }

        // debugf("pid %d keep waiting", p->pid);
        p->state = RUNNABLE;
        sched();
    }
}

void exit(int code) {
    struct proc *p = curr_proc();
    p->exit_code = code;
    infof("proc %d exit with %d\n", p->pid, code);
    freeproc(p);
    if(p->parent != 0) {
        tracef("wait for parent to clean\n");
        p->state = ZOMBIE;
    }
    debugf("before sched");
    sched();
}

int fdalloc(struct file* f) {
    struct proc* p = curr_proc();
    // fd = 0 is reserved for stdio/stdout
    for(int i = 1; i < FD_MAX; ++i) {
        if(p->files[i] == 0) {
            p->files[i] = f;
            return i;
        }
    }
    return -1;
}
