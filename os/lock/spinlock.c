#include "spinlock.h"
#include <arch/riscv.h>
#include <proc/proc.h>
#include <ucore/ucore.h>
#include <arch/timer.h>
// if you get a deadlock in debugging, define TIMEOUT so that spinlock will panic after
// some time, and you will see the debugging information
// but this will impact the performance greatly 

// #define TIMEOUT


void init_spin_lock(struct spinlock *slock) {
    slock->locked = 0;
    slock->cpu = NULL;
    slock->name = "unnamed";
}

void init_spin_lock_with_name(struct spinlock *slock, const char *name) {
    slock->locked = 0;
    slock->cpu = NULL;
    slock->name = name;
}
// Acquire the lock.
// Loops (spins) until the lock is acquired.
void acquire(struct spinlock *slock) {
    push_off(); // disable interrupts to avoid deadlock.
    if (holding(slock)) {
        errorf("spinlock timeout");
        errorf("lock \"%s\" is held by core %d, cannot be acquired", slock->name, slock->cpu - cpus);
        panic("This cpu is acquiring a acquired lock");
    }

    #ifdef TIMEOUT
    uint64 start = r_cycle();
    #endif

    while (__sync_lock_test_and_set(&slock->locked, 1) != 0)
        {
    #ifdef TIMEOUT
            uint64 now = r_cycle();
            if(now-start > SECOND_TO_CYCLE(10)){
                errorf("timeout lock name: %s, hold by cpu %d",slock->name, slock->cpu->core_id);
                panic("spinlock timeout");
            }
    #endif

        }

    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references happen strictly after the lock is acquired.
    // On RISC-V, this emits a fence instruction.
    __sync_synchronize();

    slock->cpu = mycpu();
}

// Release the lock.
void release(struct spinlock *slock) {
    // KERNEL_ASSERT(holding(slock), "a core should hold the lock if it wants to release it");
    if (!holding(slock)) {
        printf_k("Error release lock: %s\n", slock->name);
        panic("Try to release a lock when not holding it");
    }

    slock->cpu = NULL;

    // Tell the C compiler and the CPU to not move loads or stores
    // past this point, to ensure that all the stores in the critical
    // section are visible to other CPUs before the lock is released,
    // and that loads in the critical section occur strictly before
    // the lock is released.
    // On RISC-V, this emits a fence instruction.
    __sync_synchronize();

    // Release the lock, equivalent to lk->locked = 0.
    // This code doesn't use a C assignment, since the C standard
    // implies that an assignment might be implemented with
    // multiple store instructions.
    // On RISC-V, sync_lock_release turns into an atomic swap:
    //   s1 = &lk->locked
    //   amoswap.w zero, zero, (s1)
    __sync_lock_release(&slock->locked);

    pop_off();
}
// Check whether this cpu is holding the lock.
// Interrupts must be off.
int holding(struct spinlock *lk) {
    int r;
    r = (lk->locked && lk->cpu == mycpu());
    return r;
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.
void push_off(void) {
    int old = intr_get();

    intr_off();
    if (mycpu()->noff == 0) {
        mycpu()->base_interrupt_status = old;
    }
    mycpu()->noff += 1;
}

void pop_off(void) {
    struct cpu *c = mycpu();
    if (intr_get())
        panic("pop_off - interruptible");
    if (c->noff < 1)
        panic("pop_off");
    c->noff -= 1;
    if (c->noff == 0 && c->base_interrupt_status) {
        intr_on();
    }
}
