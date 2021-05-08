#include <arch/riscv.h>
#include <ucore/defs.h>
#include <proc/proc.h>
#include <file/file.h>

int
pipealloc(struct file *f0, struct file *f1)
{
    struct pipe *pi;
    pi = 0;
    if((pi = (struct pipe*)alloc_physical_page()) == 0)
        goto bad;
    memset(pi, 0, PGSIZE);
    pi->readopen = 1;
    pi->writeopen = 1;
    pi->nwrite = 0;
    pi->nread = 0;
    init_spin_lock_with_name(&pi->lock, "pipe.lock");
    f0->type = FD_PIPE;
    f0->readable = 1;
    f0->writable = 0;
    f0->pipe = pi;
    f1->type = FD_PIPE;
    f1->readable = 0;
    f1->writable = 1;
    f1->pipe = pi;
    return 0;
bad:
    if(pi)
        recycle_physical_page((char*)pi);
    return -1;
}

void
pipeclose(struct pipe *pi, int writable)
{
    acquire(&pi->lock);
    if(writable){
        pi->writeopen = 0;
    } else {
        pi->readopen = 0;
    }
    if(pi->readopen == 0 && pi->writeopen == 0){
        release(&pi->lock);
        recycle_physical_page((char*)pi);
    }else
        release(&pi->lock);
}

int
pipewrite(struct pipe *pi, uint64 addr, int n)
{
    int w = 0;
    uint64 size;
    struct proc *p = curr_proc();
    if(n <= 0) {
        panic("invalid read num");
    }
    acquire(&pi->lock);
    while(w < n){
        if(pi->readopen == 0){
            return -1;
        }
        if(pi->nwrite == pi->nread + PIPESIZE){ //DOC: pipewrite-full
            release(&pi->lock);
            yield();
            acquire(&pi->lock);
        } else {
            size = MIN(MIN(n - w, pi->nread + PIPESIZE - pi->nwrite), PIPESIZE - (pi->nwrite % PIPESIZE));
            if(copyin(p->pagetable, &pi->data[pi->nwrite % PIPESIZE], addr + w, size) < 0) {
                panic("copyin");
            }
            pi->nwrite += size;
            w += size;
        }
    }
    release(&pi->lock);

    return w;
}

int
piperead(struct pipe *pi, uint64 addr, int n)
{
    int r = 0;
    uint64 size = -1;
    struct proc *p = curr_proc();
    if(n <= 0) {
        panic("invalid read num");
    }
    acquire(&pi->lock);
    while(pi->nread == pi->nwrite) {
        if(pi->writeopen)
        {
            release(&pi->lock);
            yield();
            acquire(&pi->lock);
        }
        else
            return -1;
    }
    while(r < n && size != 0) {  //DOC: piperead-copy
        if(pi->nread == pi->nwrite)
            break;
        size = MIN(MIN(n - r, pi->nwrite - pi->nread), PIPESIZE - (pi->nread % PIPESIZE));
        if(copyout(p->pagetable, addr + r, &pi->data[pi->nread % PIPESIZE], size) < 0) {
            panic("copyout");
        }
        pi->nread += size;
        r += size;
    }
    release(&pi->lock);
    return r;
}
