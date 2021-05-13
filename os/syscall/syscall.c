#include "syscall_ids.h"
#include "syscall_impl.h"
#include <arch/riscv.h>
#include <arch/timer.h>
#include <file/fcntl.h>
#include <file/file.h>
#include <fs/fs.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <ucore/defs.h>
#include <utils/log.h>

// dispatch syscalls to different functions
void syscall() {
    struct proc *p = curr_proc();
    struct trapframe *trapframe = p->trapframe;
    int id = trapframe->a7, ret;
    uint64 args[7] = {trapframe->a0, trapframe->a1, trapframe->a2, trapframe->a3, trapframe->a4, trapframe->a5, trapframe->a6};

    // ignore read and write so that shell command don't get interrupted
    if (id != SYS_write && id != SYS_read) {
        tracecore("syscall %d args:%p %p %p %p %p %p %p", id, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
    }
    switch (id) {
    case SYS_write:
        ret = sys_write(args[0], (void *)args[1], args[2]);
        break;
    case SYS_read:
        ret = sys_read(args[0], (void *)args[1], args[2]);
        break;
    case SYS_openat:
        ret = sys_open(args[0], args[1]);
        break;
    case SYS_close:
        ret = sys_close(args[0]);
        break;
    case SYS_exit:
        ret = sys_exit(args[0]);
        break;
    case SYS_sched_yield:
        ret = sys_sched_yield();
        break;
    case SYS_setpriority:
        ret = sys_setpriority(args[0]);
        break;
    case SYS_getpriority:
        ret = sys_getpriority(args[0]);
        break;
    case SYS_getpid:
        ret = sys_getpid();
        break;
    case SYS_dup:
        ret = sys_dup((int)args[0]);
        break;
    case SYS_clone: // SYS_fork
        ret = sys_clone();
        break;
    case SYS_gettimeofday:
        ret = sys_gettimeofday((void *)args[0], args[1]);
        break;
        // case SYS_mmap:
        //     ret = sys_mmap((void *)args[0], args[1], args[2]);
        //     break;
        // case SYS_munmap:
        //     ret = sys_munmap((void *)args[0], args[1]);
        break;
    case SYS_execve:
        ret = sys_exec(args[0], (const char **)args[1]);
        break;
    case SYS_wait4:
        ret = sys_wait(args[0], args[1]);
        break;
    case SYS_times:
        ret = sys_times();
        break;
    case SYS_mknod:
        ret = sys_mknod((char *)args[0], args[1], args[2]);

        break;
    // case SYS_spawn:
    //     ret = sys_spawn((char *)args[0]);
    //     break;
    case SYS_pipe2:
        ret = sys_pipe((int(*)[2])args[0]);
        break;
    // case SYS_mailread:
    //     ret = sys_mailread((void *)args[0], args[1]);
    //     break;
    // case SYS_mailwrite:
    //     ret = sys_mailwrite(args[0], (void *)args[1], args[2]);
    //     break;
    case SYS_chdir:
        ret = sys_chdir((char *)args[0]);
        break;
    case SYS_mkdirat:
        ret = sys_mkdir((char *)args[0]);
        break;
    default:
        ret = -1;
        warnf("unknown syscall %d", id);
    }
    trapframe->a0 = ret; // return value
    if (id != SYS_write && id != SYS_read) {
        tracecore("syscall %d ret %d", id, ret);
    }
}