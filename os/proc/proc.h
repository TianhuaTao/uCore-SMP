#if !defined(PROC_H)
#define PROC_H

#include <ucore/ucore.h>

#include "exec.h"
#include <file/file.h>
#include <lock/lock.h>
#define NPROC (256)
#define KSTACK_SIZE (4096)
#define USTACK_SIZE (4096)
#define TRAPFRAME_SIZE (4096)
#define FD_MAX (16)
enum procstate {
    UNUSED = 0,
    USED,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE
};

// Per-process state
struct proc {
    struct spinlock lock;

    // p->lock must be held when using these:
    enum procstate state;  // Process state
    int pid;               // Process ID
    pagetable_t pagetable; // User page table

    uint64 ustack_bottom;        // Virtual address of user stack
    uint64 kstack;               // Virtual address of kernel stack
    struct trapframe *trapframe; // data page for trampoline.S, physical address
    struct context context;      // swtch() here to run process
    uint64 total_size;           // total memory used by this process
    uint64 heap_sz;
    struct proc *parent; // Parent process
    uint64 exit_code;
    // uint64 entry;                // app bin start address
    uint64 stride;
    uint64 priority;
    uint64 cpu_time;        // ms, user and kernel
    uint64 last_start_time; // ms
    void *waiting_target;
    struct file *files[16];
    struct mailbox mb;
};
struct proc *findproc(int pid);

struct proc *curr_proc();
int spawn(char *filename);
extern struct proc pool[NPROC];
extern struct spinlock pool_lock;

void sleep(void *waiting_target, struct spinlock *lk);
void wakeup(void *waiting_target);

void print_proc(struct proc *proc);
void forkret(void);

void proc_free_mem_and_pagetable(pagetable_t pagetable, uint64 sz);
struct proc *allocproc(void);
pagetable_t proc_pagetable(struct proc *p);
#endif // PROC_H
