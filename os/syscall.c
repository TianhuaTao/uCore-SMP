#include "defs.h"
#include "syscall_ids.h"
#include "trap.h"
#include "proc.h"
#include "log.h"
#include "timer.h"
#define min(a, b) a < b ? a : b;

/**
 * return TRUE if [sout, eout) contains [sin, ein)
 */
inline int contains(char *sout, char *eout, char *sin, char *ein)
{
    return (sout <= sin && sin < eout) && (sout <= ein && ein <= eout) && sin <= ein;
}

uint64 sys_write(int fd, char *str, uint len)
{
    if (fd != 1)
        return -1;

    struct proc *cur_pcb = curr_proc();
    char *app_start = (char *)cur_pcb->entry;
    char *app_end = app_start + 0x20000;
    char *ustack_start = (char *)cur_pcb->ustack;
    char *ustack_end = ustack_start + PAGE_SIZE;
    // debugf( "sys_write str= [%p, %p)", str, str+len);
    // debugf( "user_stack= [%p, %p)", ustack_start, ustack_end);
    // debugf( "app_address= [%p, %p)", app_start, app_end);

    if (!(
            contains(app_start, app_end, str, str + len) ||
            contains(ustack_start, ustack_end, str, str + len)))
    {
        return -1;
    }

    int size = min(strlen(str), len);
    for (int i = 0; i < size; ++i)
    {
        console_putchar(str[i]);
    }
    return size;
}

uint64 sys_exit(int code)
{
    exit(code);
    return 0;
}

uint64 sys_sched_yield()
{
    yield();
    return 0;
}

int64 sys_setpriority(int64 priority){
    if (2<=priority){
        struct proc* cur_pcb = curr_proc();
        cur_pcb -> priority = priority;
        return priority;
    }
    return -1;
}

int64 sys_gettimeofday(uint64 *timeval, int tz){
    // timeval[0] -- sec
    // timeval[0] -- usec
    uint64 us = get_time_us();
    timeval[0] = us / 1000000;
    timeval[1] = us % 1000000;
    // infof("us=%p, t0=%d, t1=%d\n", us, timeval[0],timeval[1]);

    return 0;
}

void syscall()
{
    struct trapframe *trapframe = curr_proc()->trapframe;
    int id = trapframe->a7, ret;
    tracef("syscall %d", id);
    uint64 args[7] = {trapframe->a0, trapframe->a1, trapframe->a2, trapframe->a3, trapframe->a4, trapframe->a5, trapframe->a6};
    switch (id)
    {
    case SYS_write:
        ret = sys_write(args[0], (char *)args[1], args[2]);
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
    case SYS_gettimeofday:
        ret =sys_gettimeofday((void*)args[0], args[1]);
        break;    
    default:
        ret = -1;
        infof("unknown syscall %d\n", id);
    }
    trapframe->a0 = ret;
    tracef("syscall ret %d", ret);
}
