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

char *syscall_names(int id)
{
    switch (id)
    {
    case SYS_getcwd:
        return "SYS_getcwd";
    case SYS_dup:
        return "SYS_dup";
    case SYS_mknod:
        return "SYS_mknod";
    case SYS_mkdir:
        return "SYS_mkdir";
    case SYS_link:
        return "SYS_link";
    case SYS_unlink:
        return "SYS_unlink";
    case SYS_chdir:
        return "SYS_chdir";
    case SYS_open:
        return "SYS_open";
    case SYS_close:
        return "SYS_close";
    case SYS_pipe:
        return "SYS_pipe";
    case SYS_read:
        return "SYS_read";
    case SYS_write:
        return "SYS_write";
    case SYS_fstat:
        return "SYS_fstat";
    case SYS_exit:
        return "SYS_exit";
    case SYS_waitpid:
        return "SYS_waitpid";
    case SYS_sched_yield:
        return "SYS_sched_yield";
    case SYS_kill:
        return "SYS_kill";
    case SYS_setpriority:
        return "SYS_setpriority";
    case SYS_getpriority:
        return "SYS_getpriority";
    case SYS_time_ms:
        return "SYS_time_ms";
    case SYS_gettimeofday:
        return "SYS_gettimeofday";
    case SYS_settimeofday:
        return "SYS_settimeofday";
    case SYS_getpid:
        return "SYS_getpid";
    case SYS_getppid:
        return "SYS_getppid";
    case SYS_sysinfo:
        return "SYS_sysinfo";
    case SYS_brk:
        return "SYS_brk";
    case SYS_munmap:
        return "SYS_munmap";
    case SYS_fork:
        return "SYS_fork";
    case SYS_mmap:
        return "SYS_mmap";
    case SYS_execv:
        return "SYS_execv";
    case SYS_spawn:
        return "SYS_spawn";
    case SYS_mailread:
        return "SYS_mailread";
    case SYS_mailwrite:
        return "SYS_mailwrite";
    default:
        return "?";
    }
}

#define SYSCALL_NAMES_MAX (sizeof(syscall_names) / sizeof(syscall_names[0]))
// dispatch syscalls to different functions
void syscall()
{
    struct proc *p = curr_proc();
    struct trapframe *trapframe = p->trapframe;
    int id = trapframe->a7, ret;
    uint64 args[7] = {trapframe->a0, trapframe->a1, trapframe->a2, trapframe->a3, trapframe->a4, trapframe->a5, trapframe->a6};

    // ignore read and write so that shell command don't get interrupted
    if (id != SYS_write && id != SYS_read)
    {
        char *name=syscall_names(id);
        tracecore("syscall %d (%s) args:%p %p %p %p %p %p %p", id, name ,args[0] , args[1], args[2], args[3], args[4], args[5], args[6]);
    }
    switch (id)
    {
    case SYS_write:
        ret = sys_write(args[0], (void *)args[1], args[2]);
        break;
    case SYS_read:
        ret = sys_read(args[0], (void *)args[1], args[2]);
        break;
    case SYS_open:
        ret = sys_open((char *)args[0], args[1]);
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
    case SYS_getppid:
        ret = sys_getppid();
        break;
    case SYS_dup:
        ret = sys_dup((int)args[0]);
        break;
    case SYS_fork:
        ret = sys_fork();
        break;
    case SYS_execv:
        ret = sys_execv((char *)args[0], (char **)args[1]);
        break;
    case SYS_waitpid:
        ret = sys_waitpid(args[0], (int *)args[1]);
        break;
    case SYS_time_ms:
        ret = sys_time_ms();
        break;
    case SYS_mknod:
        ret = sys_mknod((char *)args[0], args[1], args[2]);
        break;
    case SYS_pipe:
        ret = sys_pipe((int(*)[2])args[0]);
        break;
    case SYS_fstat:
        ret = sys_fstat((int)args[0], (void *)args[1]);
        break;
    case SYS_chdir:
        ret = sys_chdir((char *)args[0]);
        break;
    case SYS_mkdir:
        ret = sys_mkdir((char *)args[0]);
        break;
    case SYS_link:
        ret = sys_link((char *)args[0], (char *)args[1]);
        break;
    case SYS_unlink:
        ret = sys_unlink((char *)args[0]);
    case SYS_set_dsid:
        ret = sys_set_dsid(args[0], args[1]);
        break;
    case SYS_set_dsid_param:
        ret = sys_set_dsid_param(args[0], args[1], args[2], args[3], args[4]);
        break;
    case SYS_get_l2_traffic:
        ret = sys_get_l2_traffic(args[0]);
        break;
    default:
        ret = -1;
        warnf("unknown syscall %d", id);
    }
    trapframe->a0 = ret; // return value
    if (id != SYS_write && id != SYS_read)
    {
        tracecore("syscall %d ret %d", id, ret);
    }
}