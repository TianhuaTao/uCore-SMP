#if !defined(LOCK_H)
#define LOCK_H

#include "defs.h"

struct cpu *
mycpu(void);

struct spinlock
{
    uint locked;
    struct cpu *cpu;
};



void acquire(struct spinlock *slock);
void release(struct spinlock *slock);
void push_off(void);
void pop_off(void);
int holding(struct spinlock *lk);

void init_spin_lock(struct spinlock *slock);

#endif // LOCK_H
