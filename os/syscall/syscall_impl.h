#if !defined(SYSCALL_IMPL_H)
#define SYSCALL_IMPL_H

#include <ucore/ucore.h>

uint64 sys_read(int fd, uint64 va, uint64 len);
uint64 sys_pipe(uint64 fdarray);
uint64 sys_exit(int code);
uint64 sys_sched_yield();
uint64 sys_getpid();
uint64 sys_clone();
uint64 sys_exec(uint64 va);
uint64 sys_wait(int pid, uint64 va);
uint64 sys_times();
int64 sys_setpriority(int64 priority);
int64 sys_gettimeofday(uint64 *timeval, int tz);
uint64 sys_close(int fd);
int sys_spawn(char *filename);
uint64 sys_openat(uint64 va, uint64 omode, uint64 _flags);
int64 sys_mmap(void *start, uint64 len, int prot);
int64 sys_munmap(void *start, uint64 len);
int sys_mailread(void *buf, int len);
int sys_mailwrite(int pid, void *buf, int len);
uint64 sys_write(int fd, uint64 va, uint64 len);

#endif // SYSCALL_IMPL_H
