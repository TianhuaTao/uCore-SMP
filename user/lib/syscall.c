#include <stddef.h>
#include <ucore.h>
#include <fcntl.h>
#include "syscall.h"

#include <ucore.h>
#include <ucore_syscall_ids.h>

int execv(const char *pathname, char *const argv[])
{
    return syscall(SYS_execv, pathname, argv);
}

int exec(const char *pathname)
{
    return syscall(SYS_execv, pathname, NULL);
}

void exit(int status)
{
    syscall(SYS_exit, status);
}
ssize_t read(int fd, void *dst, size_t len)
{
    return syscall(SYS_read, fd, dst, len);
}
ssize_t write(int fd, void *src, size_t len)
{
    return syscall(SYS_write, fd, src, len);
}

pid_t getpid(void)
{
    return syscall(SYS_getpid);
}

int open(const char *pathname, int flags)
{
    return syscall(SYS_open, pathname, flags);
}

int mknod(const char *pathname, short major, short minor)
{
    return syscall(SYS_mknod, pathname, major, minor);
}

int dup(int oldfd)
{
    return syscall(SYS_dup, oldfd);
}

int sched_yield(void)
{
    return syscall(SYS_sched_yield);
}

pid_t waitpid(pid_t pid, int *wstatus)
{
    return syscall(SYS_waitpid, pid, wstatus);
}

pid_t wait(int *wstatus)
{
    return waitpid(-1, wstatus);
}

int mkdir(const char *pathname)
{
    return syscall(SYS_mkdir, pathname);
}

int close(int fd)
{
    return syscall(SYS_close, fd);
}

pid_t fork(void)
{
    return syscall(SYS_fork);
}

uint64 time_ms()
{
    return syscall(SYS_time_ms);
}

int sleep(unsigned long long time_in_ms)
{
    unsigned long long s = time_ms();
    while (time_ms() < s + time_in_ms)
    {
        sched_yield();
    }
    return 0;
}

int pipe(int pipefd[2])
{
    return syscall(SYS_pipe2, pipefd);
}

int fstat(int fd, struct stat *statbuf)
{
    return syscall(SYS_fstat, fd, statbuf);
}

int stat(const char *pathname, struct stat *statbuf)
{
    int fd;
    int r;

    fd = open(pathname, O_RDONLY);
    if (fd < 0)
        return -1;
    r = fstat(fd, statbuf);
    close(fd);
    return r;
}

int chdir(const char *path){
    return syscall(SYS_chdir, path);
}

// =============================================================
