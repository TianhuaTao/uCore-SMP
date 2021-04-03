#ifndef __UNISTD_H__
#define __UNISTD_H__

#include "stddef.h"

int open(const char*, int, int);

ssize_t read(int, void*, size_t);
ssize_t write(int, const void*, size_t);

int close(int);
pid_t getpid(void);
int sched_yield(void);
void exit(int);
int fork(void);
int exec(char*);
int wait(int, int*);
uint64 get_time();
int sleep(unsigned long long);
#endif // __UNISTD_H__
