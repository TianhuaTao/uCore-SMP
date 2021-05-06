#if !defined(LOCK_H)
#define LOCK_H

#include "defs.h"

struct cpu *
mycpu(void);

struct spinlock
{
    uint locked;
    struct cpu *cpu;

    const char *name;
};

struct mutex
{
    uint locked; // Is the lock held?
    struct spinlock guard_lock; 
    const char *name;
};


void acquire(struct spinlock *slock);
void release(struct spinlock *slock);
void push_off(void);
void pop_off(void);
int holding(struct spinlock *lk);

void init_spin_lock(struct spinlock *slock);
void init_spin_lock_with_name(struct spinlock *slock, const char* name);
void init_mutex(struct mutex *mutex);
void acquire_mutex_sleep(struct mutex *mu);
void release_mutex_sleep(struct mutex *mu);
int
holdingsleep(struct mutex *lk);
// {
//   int ret;
  
//   acquire(&lk->guard_lock);
//   ret = lk->locked && (lk->pid == myproc()->pid);
//   release(&lk->guard_lock);
//   return ret;
// }

#endif // LOCK_H
