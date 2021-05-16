#if !defined(SYSCALL_IMPL_H)
#define SYSCALL_IMPL_H

#include <ucore/ucore.h>

struct stat;

ssize_t sys_write(int fd, void *src_va, size_t len);
ssize_t sys_read(int fd, void *dst_va, size_t len);
int sys_pipe(int (*pipefd_va)[2]);
uint64 sys_exit(int code);
int64 sys_mkdir(char *path_va);
uint64 sys_sched_yield();
uint64 sys_getpid();
uint64 sys_clone();
uint64 sys_exec(uint64 va, const char **argv);
int sys_wait(int pid, uint64 exitcode_va);
uint64 sys_times();
int64 sys_setpriority(int64 priority);
int64 sys_getpriority();
int64 sys_gettimeofday(uint64 *timeval, int tz);
uint64 sys_close(int fd);
int sys_open(uint64 va, int flags);
int sys_mknod(char *path_va, short major, short minor);
int sys_dup(int oldfd);
int64 sys_chdir(char *path_va);
int sys_fstat(int fd, struct stat *statbuf_va);
// int sys_spawn(char *filename);
// int64 sys_mmap(void *start, uint64 len, int prot);
// int64 sys_munmap(void *start, uint64 len);
// int sys_mailread(void *buf, int len);
// int sys_mailwrite(int pid, void *buf, int len);

#endif // SYSCALL_IMPL_H
