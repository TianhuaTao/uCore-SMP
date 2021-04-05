#if !defined(PROC_H)
#define PROC_H

#include "ucore.h"

#include "lock.h"

#define NPROC (256)
#define KSTACK_SIZE (4096)
#define USTACK_SIZE (4096)
#define TRAPFRAME_SIZE (4096)
enum procstate
{
  UNUSED,
  USED,
  SLEEPING,
  RUNNABLE,
  RUNNING,
  ZOMBIE
};

// Per-process state
struct proc
{
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state; // Process state
  // int killed;           // If non-zero, have been killed
  int pid;              // Process ID
  pagetable_t pagetable;       // User page table
  // these are private to the process, so p->lock need not be held.
  uint64 ustack;
  uint64 kstack;               // Virtual address of kernel stack
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  uint64 sz;
  uint64 heap_sz;
  struct proc *parent;         // Parent process
  uint64 exit_code;
  uint64 entry;                // app bin start address
  uint64 stride;
  uint64 priority;
  uint64 cpu_time;        // ms, user and kernel
  uint64 last_start_time; // ms
};

struct proc *curr_proc();
int spawn(char* filename);

#endif // PROC_H
