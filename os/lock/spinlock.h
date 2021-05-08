#if !defined(SPINLOCK_H)
#define SPINLOCK_H
#include <ucore/types.h>

struct spinlock {
    uint locked;
    struct cpu *cpu;

    const char *name;
};

void acquire(struct spinlock *slock);
void release(struct spinlock *slock);
void push_off(void);
void pop_off(void);
int holding(struct spinlock *lk);

void init_spin_lock(struct spinlock *slock);
void init_spin_lock_with_name(struct spinlock *slock, const char *name);

#endif // SPINLOCK_H
