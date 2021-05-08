#if !defined(MUTEX_H)
#define MUTEX_H
#include "spinlock.h"

struct mutex {
    uint locked; // Is the lock held?
    struct spinlock guard_lock;
    const char *name;
    int pid;
};

void init_mutex(struct mutex *mutex);
void acquire_mutex_sleep(struct mutex *mu);
void release_mutex_sleep(struct mutex *mu);
int holdingsleep(struct mutex *lk);

#endif // MUTEX_H
