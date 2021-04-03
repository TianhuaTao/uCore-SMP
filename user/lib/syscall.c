#include <stddef.h>
#include <unistd.h>

#include "syscall.h"

int open(const char *path, int flags, int mode) {
    return syscall(SYS_openat, path, flags, mode);
}

int close(int fd) {
    return syscall(SYS_close, fd);
}

ssize_t read(int fd, void *buf, size_t len) {
    return syscall(SYS_read, fd, buf, len);
}

ssize_t write(int fd, const void *buf, size_t len) {
    return syscall(SYS_write, fd, buf, len);
}

int getpid(void) {
    return syscall(SYS_getpid);
}

int sched_yield(void) {
    return syscall(SYS_sched_yield);
}

int fork(void) {
    return syscall(SYS_clone);
}

void exit(int code) {
    syscall(SYS_exit, code);
}

int wait(int pid, int* code) {
    return syscall(SYS_wait4, pid, code);
}

int exec(char* name) {
    return syscall(SYS_execve, name);
}

uint64 get_time() {
    return syscall(SYS_times);
}

int sleep(unsigned long long time) {
    unsigned long long s = get_time();
    while(get_time() < s + time) {
        sched_yield();
    }
    return 0;
}