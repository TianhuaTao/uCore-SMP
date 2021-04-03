#if !defined(DEFS_H)
#define DEFS_H

#include "types.h"
struct context;
struct proc;

// panic.c
void loop();
void panic(char *);

// sbi.c
void console_putchar(int);
int console_getchar();
void shutdown();
void set_timer(uint64 stime);

// console.c
void consoleinit();
void consputc(int);

// printf.c
void printf(char *, ...);
void
printfinit(void);

// trap.c
void trapinit();
void usertrapret();
void set_usertrap();
void set_kerneltrap();

// string.c
int memcmp(const void *, const void *, uint);
void *memmove(void *, const void *, uint);
void *memset(void *, int, uint);
char *safestrcpy(char *, const char *, int);
int strlen(const char *);
int strncmp(const char *, const char *, uint);
char *strncpy(char *, const char *, int);

// syscall.c
void syscall();

// swtch.S
void swtch(struct context *, struct context *);

// batch.c
// int finished();
void batchinit();
int run_all_app();

// proc.c
struct proc *curr_proc();
void exit(int);
void procinit();
void scheduler(); // __attribute__((noreturn));
void sched();
void yield();
struct proc* allocproc();
void init_scheduler();
// timer.c
uint64 get_cycle();
void timerinit();
void set_next_timer();

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#define PAGE_SIZE (4096)

#endif // DEFS_H
