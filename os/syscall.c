#include "defs.h"
#include "syscall_ids.h"
#include "trap.h"
#include "proc.h"
#include "log.h"
#include "timer.h"
#include "riscv.h"
#include "fcntl.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "syscall_impl.h"

/**
 * return TRUE if [sout, eout) contains [sin, ein)
 */
inline int contains(char *sout, char *eout, char *sin, char *ein)
{
    return (sout <= sin && sin < eout) && (sout <= ein && ein <= eout) && sin <= ein;
}

void syscall()
{
    struct proc *p = curr_proc();
    struct trapframe *trapframe = p->trapframe;
    int id = trapframe->a7, ret;
    uint64 args[7] = {trapframe->a0, trapframe->a1, trapframe->a2, trapframe->a3, trapframe->a4, trapframe->a5, trapframe->a6};
    if (id != SYS_write && id != SYS_read)
    {
        debugcore("syscall %d args:%p %p %p %p %p %p %p", id, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
    }
    switch (id)
    {
    case SYS_write:
        ret = sys_write(args[0], args[1], args[2]);
        break;
    case SYS_read:
        ret = sys_read(args[0], args[1], args[2]);
        break;
    case SYS_openat:
        ret = sys_openat(args[0], args[1], args[2]);
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
        ret = -1;
        warnf("not implemented\n");
        break;
    case SYS_getpid:
        ret = sys_getpid();
        break;
    case SYS_clone: // SYS_fork
        ret = sys_clone();
        break;
    case SYS_gettimeofday:
        ret = sys_gettimeofday((void *)args[0], args[1]);
        break;
    case SYS_mmap:
        ret = sys_mmap((void *)args[0], args[1], args[2]);
        break;
    case SYS_munmap:
        ret = sys_munmap((void *)args[0], args[1]);
        break;
    case SYS_execve:
        ret = sys_exec(args[0]);
        break;
    case SYS_wait4:
        ret = sys_wait(args[0], args[1]);
        break;
    case SYS_times:
        ret = sys_times();
        break;
    case SYS_spawn:
        ret = sys_spawn((char *)args[0]);
        break;
    case SYS_pipe2:
        ret = sys_pipe(args[0]);
        break;
    case SYS_mailread:
        ret = sys_mailread((void *)args[0], args[1]);
        break;
    case SYS_mailwrite:
        ret = sys_mailwrite(args[0], (void *)args[1], args[2]);
        break;
    default:
        ret = -1;
        warnf("unknown syscall %d\n", id);
    }
    trapframe->a0 = ret;
    if (id != SYS_write && id != SYS_read)
    {
        debugcore("syscall ret %d\n", ret);
    }
}