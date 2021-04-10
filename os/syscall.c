#include "defs.h"
#include "syscall_ids.h"
#include "trap.h"
#include "proc.h"
#include "log.h"
#include "timer.h"
#include "riscv.h"
#include "file.h"
#define min(a, b) a < b ? a : b;

uint64 console_write(uint64 va, uint64 len) {
    struct proc *p = curr_proc();
    char str[200];
    int size = copyinstr(p->pagetable, str, va, MIN(len, 200));
    for(int i = 0; i < size; ++i) {
        console_putchar(str[i]);
    }
    return size;
}

uint64 console_read(uint64 va, uint64 len) {
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

/**
 * return TRUE if [sout, eout) contains [sin, ein)
 */
inline int contains(char *sout, char *eout, char *sin, char *ein)
{
    return (sout <= sin && sin < eout) && (sout <= ein && ein <= eout) && sin <= ein;
}

uint64 sys_write(int fd, uint64 va, uint64 len) {
    if(fd == 1) {
        return console_write(va, len);
    }
    if(fd>=FD_MAX){
        return -1;
    }
    infof("fd=%d\n", fd);
    struct proc *p = curr_proc();
    struct file *f = p->files[fd];
    if(f==0){
        return -1;
    }
    if(f->type == FD_PIPE) {
        debugf("write to pipe at %p\n", f->pipe);
        return pipewrite(f->pipe, va, len);
    }
    errorf("unknown file type %d\n", f->type);
    panic("syswrite: unknown file type\n");
    return -1;
}

uint64 sys_read(int fd, uint64 va, uint64 len) {
    if(fd == 0) {
        return console_read(va, len);
    }
    if(fd>=FD_MAX){
        return -1;
    }
    struct proc *p = curr_proc();
    struct file *f = p->files[fd];
    if(f==0){
        return -1;
    }
    if(f->type == FD_PIPE) {
        debugf("read to pipe at %p\n", f->pipe);
        return piperead(f->pipe, va, len);
    }
    errorf("unknown file type %d\n", f->type);
    panic("sysread: unknown file type\n");
    return -1;
}

uint64
sys_pipe(uint64 fdarray) {
    infof("init pipe\n");
    struct proc *p = curr_proc();
    uint64 fd0, fd1;
    struct file* f0, *f1;
    if(f0 < 0 || f1 < 0) {
        return -1;
    }
    f0 = filealloc();
    f1 = filealloc();
    if(pipealloc(f0, f1) < 0)
        return -1;
    fd0 = fdalloc(f0);
    fd1 = fdalloc(f1);
    if(copyout(p->pagetable, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
       copyout(p->pagetable, fdarray+sizeof(uint64), (char *)&fd1, sizeof(fd1)) < 0){
        fileclose(f0);
        fileclose(f1);
        p->files[fd0] = 0;
        p->files[fd1] = 0;
        return -1;
    }
    return 0;
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
uint64 sys_close(int fd) {
    if(fd == 0)
        return 0;
    struct proc *p = curr_proc();
    struct file *f = p->files[fd];
    if(f->type != FD_PIPE) {
        errorf("unknown file type %d\n", f->type);
        panic("fileclose: unknown file type\n");
    }
    fileclose(f);
    p->files[fd] = 0;
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
int sys_mailread(void* buf, int len){
    infof("mailread\n");
    if(len>256){
        len = 256;
    }
    if(len<0){
        return -1;
    }
    struct proc* p = curr_proc();
    
    struct mailbox *inbox = &(p->mb);

    acquire(&inbox->lock);
    if(len == 0){
        for (int i = 0; i < MAX_MAIL_IN_BOX; i++)
        {
            if(inbox->valid[i]){
                release(&inbox->lock);
                return 0;
            }
        }
        release(&inbox->lock);
        return -1;
    }


    // read head mail
    int head_idx = inbox->head;
    if (inbox->valid[head_idx]){
        int msg_len = inbox->length[head_idx];
        int copy_len = min(msg_len, len);
        int eret = copyout(p->pagetable, (uint64)buf, inbox->mailbuf[head_idx], copy_len);
        if (eret <0){
            infof("copyout failed\n");
            release(&inbox->lock);
            return -1;
        }
        inbox->valid[inbox->head] = 0;
        inbox->head+=1;
        inbox->head = (inbox->head) % MAX_MAIL_IN_BOX;
        release(&inbox->lock)
        infof("read mail %d bytes\n", copy_len);
        return copy_len;
    }else{
        // mail box is empty
        release(&inbox->lock);
        infof("mail box is empty\n");
        return -1;
    }


}

int sys_mailwrite(int pid, void*buf, int len){
    infof("mailwrite\n");
    if(len>256){
        len = 256;
    }
    if(len<0){
        return -1;
    }
    struct proc* cur_p = curr_proc();

    struct proc* p = findproc(pid);
    if( p == 0 ){return -1;}
    struct mailbox *dest = &(p->mb);

    acquire(&dest->lock);

    if (len == 0 ){
        for (int i = 0; i < MAX_MAIL_IN_BOX; i++)
        {
            if(!(dest->valid[i])){
                // empty slot
                release(&dest->lock);
                return 0;
            }
        }
        release(&dest->lock);
        return -1;
    }


    // write mail
    int head_idx = dest->head;
    for (int j = 0; j < MAX_MAIL_IN_BOX; j++)
    {
        if (dest->valid[j] != 0){
            // not empty, find next
        }else{
            // empty, write to this one
            int eret = copyin(cur_p->pagetable, dest->mailbuf[j], (uint64)buf, len);
            if(eret <0){
                infof("copyin failed\n");
                release(&dest->lock);
                return -1;
            }
            dest->valid[j] = 1;
            dest->length[j]=len;
            release(&dest->lock);
            return len;
        }
        head_idx+=1;
        head_idx = head_idx % MAX_MAIL_IN_BOX;
    }

    // all filled
    release(&dest->lock);
    return -1;
    
}

void syscall() {
    struct proc *p = curr_proc();
    struct trapframe *trapframe = p->trapframe;
    int id = trapframe->a7, ret;
    uint64 args[7] = {trapframe->a0, trapframe->a1, trapframe->a2, trapframe->a3, trapframe->a4, trapframe->a5, trapframe->a6};
    if (id != SYS_write && id != SYS_read){
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
        case SYS_pipe2:
            ret = sys_pipe(args[0]);
            break;
        case SYS_close:
            ret = sys_close(args[0]);
            break;
        case SYS_mailread:
            ret = sys_mailread((void*)args[0], args[1]);
            break;
        case SYS_mailwrite:
            ret = sys_mailwrite(args[0], (void*)args[1], args[2]);
            break;
        default:
            ret = -1;
            warnf("unknown syscall %d\n", id);
    }
    trapframe->a0 = ret;
    if (id != SYS_write && id != SYS_read){debugf("syscall ret %d\n", ret);}
}