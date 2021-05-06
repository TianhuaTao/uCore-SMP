#include "lock.h"
#include "ucore.h"
#include "riscv.h"
#include "proc.h"
void init_spin_lock(struct spinlock *slock)
{
  slock->locked = 0;
  slock->cpu = NULL;
  slock->name = "unnamed";
}

void init_spin_lock_with_name(struct spinlock *slock, const char *name){
    slock->locked = 0;
    slock->cpu = NULL;
    slock->name = name;
}
void init_mutex(struct mutex *mutex){
    init_spin_lock(&mutex->guard_lock);
    mutex->locked = FALSE;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
void acquire(struct spinlock *slock)
{
  push_off(); // disable interrupts to avoid deadlock.
  if (holding(slock)){
      errorf("lock \"%s\" is held by core %d, cannot be acquired", slock->name, slock->cpu - cpus);
      panic("This cpu is acquiring a acquired lock");
  }
  // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
  //   a5 = 1
  //   s1 = &slock->locked
  //   amoswap.w.aq a5, a5, (s1)
  while (__sync_lock_test_and_set(&slock->locked, 1) != 0)
    ;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen strictly after the lock is acquired.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();

  slock->cpu = mycpu();
}

// Release the lock.
void release(struct spinlock *slock)
{
  if (!holding(slock))
    panic("release");

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
int holding(struct spinlock *lk)
{
  int r;
  r = (lk->locked && lk->cpu == mycpu());
  return r;
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.

void push_off(void)
{
  int old = intr_get();

  intr_off();
  if (mycpu()->noff == 0)
  {
    mycpu()->maintence = old;  
  }
  mycpu()->noff += 1;
}

void pop_off(void)
{
  struct cpu *c = mycpu();
  if (intr_get())
    panic("pop_off - interruptible");
  if (c->noff < 1)
    panic("pop_off");
  c->noff -= 1;
  if (c->noff == 0 && c->maintence){
    intr_on();
  }
}

void acquire_mutex_sleep(struct mutex *mu){
    acquire(&mu->guard_lock);
    while (mu->locked) {
        sleep(mu, &mu->guard_lock);
    }
    mu->locked = 1;
    // mu->pid = myproc()->pid;
    release(&mu->guard_lock);
}
void release_mutex_sleep(struct mutex *mu){
    acquire(&mu->guard_lock);
    mu->locked = 0;
    // mu->pid = 0;
    wakeup(mu);
    release(&mu->guard_lock);
}


 