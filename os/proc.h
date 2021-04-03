#if !defined(PROC_H)
#define PROC_H

#include "ucore.h"

#include "lock.h"

#define NPROC (32)

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
  int killed;           // If non-zero, have been killed
  int pid;              // Process ID
  // these are private to the process, so p->lock need not be held.
  uint64 ustack;
  uint64 kstack;               // Virtual address of kernel stack
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  uint64 entry;                // app bin start address
  uint64 stride;
  uint64 priority;
  uint64 cpu_time;        // ms, user and kernel
  uint64 last_start_time; // ms
};

struct proc *curr_proc();

#endif // PROC_H
