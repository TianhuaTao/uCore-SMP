#include <arch/riscv.h>
#include <arch/timer.h>
#include <file/file.h>
#include <mem/memory_layout.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <ucore/defs.h>
#include <utils/log.h>
#include <mem/shared.h>
struct proc pool[NPROC];

__attribute__((aligned(16))) char kstack[NPROC][KSTACK_SIZE];
// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;
struct spinlock pool_lock;
// struct spinlock proc_tree_lock;
struct
{
    int pid; // the next available pid
    struct spinlock lock;
} next_pid;
struct proc *curr_proc() {
    // if the timer were to interrupt and cause the thread to yield and then move to a different CPU
    // a previously returned value would no longer be correct.
    push_off();
    struct cpu *c = mycpu();
    struct proc *p = c->proc;
    pop_off();
    return p;
}

struct proc *findproc(int pid) {
    struct proc *p = NULL;
    acquire(&pool_lock);
    for (p = pool; p < &pool[NPROC]; p++) {
        if (p->state != UNUSED && p->pid == pid) {
            break;
        }
    }
    release(&pool_lock);
    return p;
}

void procinit(void) {
    struct proc *p;
    init_spin_lock_with_name(&pool_lock, "pool_lock");
    init_spin_lock_with_name(&wait_lock, "wait_lock");
    // init_spin_lock_with_name(&proc_tree_lock, "proc_tree_lock");
    for (p = pool; p < &pool[NPROC]; p++) {
        init_spin_lock_with_name(&p->lock, "proc.lock");
        p->state = UNUSED;

    }

    next_pid.pid = 1;
    init_spin_lock_with_name(&next_pid.lock, "next_pid.lock");
}

int alloc_pid() {
    acquire(&next_pid.lock);
    int pid = next_pid.pid;
    next_pid.pid++;
    release(&next_pid.lock);
    return pid;
}
pagetable_t proc_pagetable(struct proc *p) {
    pagetable_t pagetable;

    // An empty page table.
    pagetable = create_empty_user_pagetable();
    if (pagetable == NULL)
        panic("cannot create empty user pagetable");

    if (mappages(pagetable, TRAMPOLINE, PGSIZE,
                 (uint64)trampoline, PTE_R | PTE_X) < 0) {
        free_pagetable_pages(pagetable);
        return 0;
    }

    if ((p->trapframe = (struct trapframe *)alloc_physical_page()) == 0) {
        panic("alloc trapframe page failed\n");
    }

    // map the trapframe just below TRAMPOLINE, for trampoline.S.
    if (mappages(pagetable, TRAPFRAME, PGSIZE,
                 (uint64)(p->trapframe), PTE_R | PTE_W) < 0) {
        ;
        panic("Can not map TRAPFRAME");
    }

    return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_free_mem_and_pagetable(struct proc* p) {
    uvmunmap(p->pagetable, TRAMPOLINE, 1, FALSE);  // unmap, don't recycle physical, shared
    uvmunmap(p->pagetable, TRAPFRAME, 1, TRUE);   // unmap, should recycle physical
    p->trapframe = NULL;

    // unmap shared memory
    for (int i = 0; i < MAX_PROC_SHARED_MEM_INSTANCE; i++)
    {
        if (p->shmem[i])
        { // active shared memory
            debugcore("free shared mem");
            uvmunmap(p->pagetable, (uint64)p->shmem_map_start[i], p->shmem[i]->page_cnt, FALSE);
            // debugcore("free page = %d", get_free_page_count());
            drop_shared_mem(p->shmem[i]);
            p->shmem[i]=NULL;
            p->shmem_map_start[i] = 0;
            // debugcore("free page = %d", get_free_page_count());

        }
    }

    free_user_mem_and_pagetables(p->pagetable, p->total_size);
    p->pagetable = NULL;
    p->total_size = 0;
}



/**
 * @brief clean a process struct
 * The only useful action is "p->state = UNUSED"
 * the others are just for safety
 * should hold p->lock
 * 
 * @param p the proc
 */
void freeproc(struct proc *p) {
    KERNEL_ASSERT(holding(&p->lock), "should lock the process to free");

    KERNEL_ASSERT(p->trapframe == NULL, "p->trapfram is pointing somewhere, did you forget to free trapframe?");
    KERNEL_ASSERT(p->pagetable == NULL, "p->pagetable is pointing somewhere, did you forget to free pagetable?");
    KERNEL_ASSERT(p->cwd == NULL, "p->cwd is not NULL, did you forget to release the inode?");
    KERNEL_ASSERT(p->waiting_target == NULL, "p->cwd is waiting something");
    KERNEL_ASSERT(p->total_size == 0, "memory not freed");
    KERNEL_ASSERT(p->heap_sz == 0, "heap not freed");


    p->state = UNUSED;  // very important
    p->pid = 0;
    p->killed = FALSE;
    p->parent = NULL;
    p->exit_code = 0;
    p->parent = NULL;
    p->ustack_bottom = 0;
    p->kstack = 0;
    memset(&p->context, 0, sizeof(p->context));
    p->stride = 0;
    p->priority = 0;
    p->cpu_time = 0;
    p->last_start_time = 0;
    for (int i = 0; i < FD_MAX; i++)
    {
        KERNEL_ASSERT(p->files[i] == NULL, "some file is not closed");
    }
    memset(p->name, 0, PROC_NAME_MAX);
    

    // now everything should be clean
    // still holding the lock
}

/**
 * @brief Allocate a unused proc in the pool
 * and it's initialized to some extend
 * 
 * these are not initialized or you should set them:
 * 
 * parent           NULL
 * ustack_bottom    0
 * total_size       0
 * cwd              NULL
 * name             ""
 * next_shmem_addr  0
 * 
 * @return struct proc* p with lock 
 */
struct proc *alloc_proc(void) {
    struct proc *p;
    acquire(&pool_lock);
    for (p = pool; p < &pool[NPROC]; p++) {
        if (p->state == UNUSED) {
            acquire(&p->lock);
            release(&pool_lock);
            goto found;
        }
    }
    release(&pool_lock);
    return NULL;

found:
    p->pid = alloc_pid();
    p->state = USED;
    p->total_size = 0;
    p->heap_sz = 0;
    p->killed = FALSE;
    p->waiting_target = NULL;
    p->exit_code = -1;
    p->parent = NULL;
    p->ustack_bottom = 0;
    p->pagetable = proc_pagetable(p);
    if (p->pagetable == NULL) {
        errorf("failed to create user pagetable");
        freeproc(p);
        release(&p->lock);
        return NULL;
    }
    memset(&p->context, 0, sizeof(p->context));
    p->kstack = (uint64)kstack[p - pool];
    memset((void *)p->kstack, 0, KSTACK_SIZE);

    p->context.ra = (uint64)forkret; // used in swtch()
    p->context.sp = p->kstack + KSTACK_SIZE;

    p->stride = 0;
    p->priority = 16;
    p->cpu_time = 0;
    p->last_start_time = 0;
    for (int i = 0; i < FD_MAX; i++) {
        p->files[i] = NULL;
    }
    p->cwd = NULL;
    p->name[0] = '\0';
    for (int i = 0; i < MAX_PROC_SHARED_MEM_INSTANCE; i++)
    {
       p->shmem[i] = NULL;
       p->shmem_map_start[i]= 0;
    }
    p->next_shmem_addr = 0;
    
    return p;
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void) {
    pushtrace(0x3200);
    static int first = TRUE;
    // Still holding p->lock from scheduler.
    release(&curr_proc()->lock);

    if (first) {
        // File system initialization must be run in the context of a
        // regular process (e.g., because it calls sleep), and thus cannot
        // be run from main().
        first = FALSE;
        fsinit();
    }

    usertrapret();
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void switch_to_scheduler(void) {
    int base_interrupt_status;
    uint64 old_ra = r_ra();
    pushtrace(0x3006);
    struct proc *p = curr_proc();
    KERNEL_ASSERT(p->state != RUNNING, "current proc shouldn't be running");
    KERNEL_ASSERT(holding(&p->lock), "should hold currernt proc's lock"); // holding currernt proc's lock
    KERNEL_ASSERT(mycpu()->noff == 1, "");                                // and it's the only lock
    KERNEL_ASSERT(!intr_get(), "interrput should be off");                // interrput is off

    base_interrupt_status = mycpu()->base_interrupt_status;
    // debugcore("in switch_to_scheduler before swtch base_interrupt_status=%d", base_interrupt_status);
    uint64 old_sp = r_sp();
    swtch(&p->context, &mycpu()->context); // will goto scheduler()
    uint64 new_sp = r_sp();
    // debugcore("in switch_to_scheduler after swtch");
    mycpu()->base_interrupt_status = base_interrupt_status;
    pushtrace(0x3020);
    pushtrace(old_sp);
    pushtrace(new_sp);
    pushtrace(old_ra);
    pushtrace(*(uint64*)(r_sp()+40));
}

/**
 * @brief For debugging.
 * 
 * @param proc 
 */
void print_proc(struct proc *proc) {
    printf_k("* ---------- PROC INFO ----------\n");
    printf_k("* pid:                %d\n", proc->pid);
    printf_k("* status:             ");
    if (proc->state == UNUSED) {
        printf_k("UNUSED\n");
    } else if (proc->state == USED) {
        printf_k("USED\n");
    } else if (proc->state == SLEEPING) {
        printf_k("SLEEPING\n");
    } else if (proc->state == RUNNING) {
        printf_k("RUNNING\n");
    } else if (proc->state == ZOMBIE) {
        printf_k("ZOMBIE\n");
    } else if (proc->state == RUNNABLE) {
        printf_k("RUNNABLE\n");
    } else {
        printf_k("UNKNOWN\n");
    }
    printf_k("* locked:             %d\n", proc->lock.locked);
    printf_k("* killed:             %d\n", proc->killed);
    printf_k("* pagetable:          %p\n", proc->pagetable);
    printf_k("* waiting target:     %p\n", proc->waiting_target);
    printf_k("* exit_code:          %d\n", proc->exit_code);
    printf_k("*\n");

    printf_k("* parent:             %p\n", proc->parent);
    printf_k("* ustack_bottom:      %p\n", proc->ustack_bottom);
    printf_k("* kstack:             %p\n", proc->kstack);
    printf_k("* trapframe:          %p\n", proc->trapframe);
    if(proc->trapframe){
        printf_k("*     ra:             %p\n", proc->trapframe->ra);
        printf_k("*     sp:             %p\n", proc->trapframe->sp);
        printf_k("*     epc:            %p\n", proc->trapframe->epc);
    }
    printf_k("* context:            \n");
    printf_k("*     ra:             %p\n", proc->context.ra);
    printf_k("*     sp:             %p\n", proc->context.sp);
    printf_k("* total_size:         %p\n", proc->total_size);
    printf_k("* heap_sz:            %p\n", proc->heap_sz);
    printf_k("* stride:             %p\n", proc->stride);
    printf_k("* priority:           %p\n", proc->priority);
    printf_k("* cpu_time:           %p\n", proc->cpu_time);
    printf_k("* last_time:          %p\n", proc->last_start_time);
    printf_k("* files:              \n");
    for (int i = 0; i < FD_MAX; i++) {
        if (proc->files[i] != NULL) {
            if (i < 10) {
                printf_k("*     files[ %d]:      %p\n", i, proc->files[i]);
            } else {
                printf_k("*     files[%d]:      %p\n", i, proc->files[i]);
            }
        }
    }
    printf_k("* files:              \n");
    printf_k("* cwd:                %p\n", proc->cwd);
    printf_k("* name:               %s\n", proc->name);

    printf_k("* -------------------------------\n");
    printf_k("\n");
}


/**
 * Allocate a file descriptor of this process for the given file
 */
int fdalloc(struct file *f) {
    struct proc *p = curr_proc();

    for (int i = 0; i < FD_MAX; ++i) {
        if (p->files[i] == 0) {
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

    tracecore("sleep");
    // Must acquire p->lock in order to
    // change p->state and then call switch_to_scheduler.
    // Once we hold p->lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup locks p->lock),
    // so it's okay to release lk.
    // print_proc(p);

    acquire(&p->lock); 

    release(lk);

    // Go to sleep.
    p->waiting_target = waiting_target;
    p->state = SLEEPING;

    switch_to_scheduler();
    pushtrace(0x3031);

    // Tidy up.
    p->waiting_target = NULL;

    // Reacquire original lock.

    release(&p->lock);

    acquire(lk);

    // debugcore("sleep end");
}

struct file *get_proc_file_by_fd(struct proc *p, int fd) {
    if (p == NULL) {
        panic("get_proc_file_by_fd: p is NULL");
    }

    if (fd < 0 || fd >= FD_MAX) {
        return NULL;
    }

    return p->files[fd];
}
