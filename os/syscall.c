#include "defs.h"
#include "syscall_ids.h"
#include "trap.h"
#include "proc.h"
#include "log.h"
#include "timer.h"
#include "riscv.h"
#define min(a, b) a < b ? a : b;

/**
 * return TRUE if [sout, eout) contains [sin, ein)
 */
inline int contains(char *sout, char *eout, char *sin, char *ein)
{
    return (sout <= sin && sin < eout) && (sout <= ein && ein <= eout) && sin <= ein;
}

uint64 sys_write(int fd, uint64 va, uint len) 
{
    if (fd != 1)
        return -1;
    struct proc *p = curr_proc();
    char str[200];
    int size = copyinstr(p->pagetable, str, va, MIN(len, 200));
    // debug("size = %d\n", size);
    for(int i = 0; i < size; ++i) {
        console_putchar(str[i]);
    }
    return size;
}

uint64 sys_read(int fd, uint64 va, uint64 len) {
    if (fd != 0)
        return -1;
    // read from stdin is blocking
    struct proc *p = curr_proc();
    char str[200];
    for(int i = 0; i < len; ++i) {
        int c;
        do
        {
           c = console_getchar();
        } while (c==-1);
        str[i] = c;
    }
    copyout(p->pagetable, va, str, len);
    return len;
}


uint64 sys_exit(int code)
{
    infof("sys_exit");
    exit(code);
    return 0;
}

uint64 sys_sched_yield()
{
    infof("sched_yield");
    yield();
    return 0;
}
uint64 sys_getpid() {
    return curr_proc()->pid;
}

uint64 sys_clone() {
    return fork();
}

uint64 sys_exec(uint64 va) {
    struct proc* p = curr_proc();
    char name[200];
    copyinstr(p->pagetable, name, va, 200);
    infof("sys_exec %s\n", name);
    return exec(name);
}

uint64 sys_wait(int pid, uint64 va) {
    struct proc* p = curr_proc();
    int* code = (int*)useraddr(p->pagetable, va);
    return wait(pid, code);
}

uint64 sys_times() {
    return get_time_ms();
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
    uint64 timeval_ker[2];
    timeval_ker[0] = us / 1000000;
    timeval_ker[1] = us % 1000000;
    copyout(curr_proc()->pagetable, (uint64) timeval , (char*)timeval_ker, sizeof(timeval_ker));
    // info("us=%p, t0=%d, t1=%d\n", us, timeval[0],timeval[1]);

    return 0;
}

int64 sys_mmap(void* start, uint64 len, int prot){
    if(len ==0) return 0;

    if(len >1024*1024*1024){
        return -1;
    }
    uint64 aligned_len = PGROUNDUP(len);

    uint64 offset = (uint64)start & 0xfff;
    if (offset != 0) {
        return -1;
    }
    if( (prot & ~0x7) != 0){
        return -1;
    }
    if( (prot & 0x7) == 0){
        return -1;
    }
    struct proc* curr_pcb = curr_proc();
    uint64 map_size = 0;
    while (aligned_len>0)
    {
        void* pa = kalloc();
        // int PER_R = prot & 1;
        // int PER_W = prot & 2;
        // int PER_X = prot & 4;

        if (map1page(curr_pcb->pagetable, (uint64)start,
                        (uint64)pa, PTE_U|(prot<<1)) < 0) {
            debugf("sys_mmap mappages fail\n");
            return -1;
        }
        aligned_len-= PGSIZE;
        start+=PGSIZE;
        map_size+=PGSIZE;
    }
    
    if(aligned_len!=0){
        panic("aligned_len != 0");
    }
    // TODO: add size to proc.size
    debugf("map_size=%p\n", map_size);
    return map_size;

}

int64 sys_munmap(void* start, uint64 len){
    uint64 va = (uint64) start;
    uint64 a;
    pte_t *pte;
    pagetable_t pagetable = curr_proc()->pagetable;

    if (((uint64)start % PGSIZE) != 0){
        return -1;
    }
    int npages= PGROUNDUP(len) / PGSIZE;

    for (a = va; a < va + npages * PGSIZE; a += PGSIZE) {
        if ((pte = walk(pagetable, a, 0)) == 0){
            infof("uvmunmap: walk\n");
            return -1;
        }
        if ((*pte & PTE_V) == 0){
            infof("uvmunmap: not mapped\n");
            return -1;
        }
        if (PTE_FLAGS(*pte) == PTE_V){
            infof("uvmunmap: not a leaf\n");
            return -1;
        }
        
        uint64 pa = PTE2PA(*pte);
        kfree((void *) pa);
        
        *pte = 0;
    }
    return npages* PGSIZE;
}

int sys_spawn(char* filename){
    return spawn(filename);
}

void syscall() {
    struct proc *p = curr_proc();
    struct trapframe *trapframe = p->trapframe;
    int id = trapframe->a7, ret;
    uint64 args[7] = {trapframe->a0, trapframe->a1, trapframe->a2, trapframe->a3, trapframe->a4, trapframe->a5, trapframe->a6};
    if (id != SYS_write){
    debugf("syscall %d args:%p %p %p %p %p %p %p\n", id, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);}
    switch (id) {
        case SYS_write:
            ret = sys_write(args[0], args[1], args[2]);
            break;
        case SYS_read:
            ret = sys_read(args[0], args[1], args[2]);
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
            debugf("sys_clone");
            ret = sys_clone();
            break;
        case SYS_gettimeofday:
            ret =sys_gettimeofday((void*)args[0], args[1]);
            break; 
        case SYS_mmap:
            ret =sys_mmap((void*)args[0], args[1], args[2]);
            break; 
        case SYS_munmap:
            ret = sys_munmap((void*)args[0], args[1]);
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
            ret = sys_spawn((char*)args[0]);
            break;
        default:
            ret = -1;
            warnf("unknown syscall %d\n", id);
    }
    trapframe->a0 = ret;
    if (id != SYS_write){debugf("syscall ret %d\n", ret);}
}