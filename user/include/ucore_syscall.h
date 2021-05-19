#if !defined(UCORE_SYSCALL_H)
#define UCORE_SYSCALL_H

int execv(const char *pathname, char *const argv[]);

int exec(const char *pathname);

void exit(int status);

ssize_t read(int fd, void *dst, size_t len);

ssize_t write(int fd, void *src, size_t len);

pid_t getpid(void);

pid_t getppid(void);

int open(const char *pathname, int flags);

int mknod(const char *pathname, short major, short minor);

int dup(int oldfd);

int sched_yield(void);

pid_t waitpid(pid_t pid, int *wstatus);

pid_t wait(int *wstatus);

int mkdir(const char *pathname);

int close(int fd);

pid_t fork(void);

uint64 time_ms();

int sleep(unsigned long long time_in_ms);

int pipe(int pipefd[2]);

int fstat(int fd, struct stat *statbuf);

int stat(const char *pathname, struct stat *statbuf);

int chdir(const char *path);

int link(const char *oldpath, const char *newpath);

int unlink(const char *pathname);

int64 setpriority(int64 priority);

int64 getpriority();

void* sharedmem(char* name, size_t len);
//////////////////////


#endif // UCORE_SYSCALL_H
