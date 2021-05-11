#include "syscall_impl.h"
#include <arch/timer.h>
#include <proc/proc.h>
#define min(a, b) a < b ? a : b;
uint64 console_write(uint64 va, uint64 len) {
    struct proc *p = curr_proc();
    char str[200];
    int size = copyinstr(p->pagetable, str, va, MIN(len, 200));
    for (int i = 0; i < size; ++i) {
        console_putchar(str[i]);
    }
    return size;
}

uint64 console_read(uint64 va, uint64 len) {
    struct proc *p = curr_proc();
    char str[200];
    for (int i = 0; i < MIN(len, 200); ++i) {
        int c;
        do {
            c = console_getchar();
        } while (c == -1);
        str[i] = c;
    }
    copyout(p->pagetable, va, str, len);
    return len;
}

uint64 sys_read(int fd, uint64 dst_va, uint64 len) {
    // stdin
    if (fd == STDIN) {
        return console_read(dst_va, len);
    }
    if (fd >= FD_MAX || fd < 0 || fd == STDOUT || fd == STDERR) {
        return -1;
    }
    struct proc *current_proc = curr_proc();
    struct file *f = current_proc->files[fd];
    if (f == NULL) {
        return -1;
    }
    if (f->type == FD_PIPE) {
        return piperead(f->pipe, dst_va, len);
    } else if (f->type == FD_INODE) {
        return fileread(f, dst_va, len);
    }
    errorf("unknown file type %d\n", f->type);
    panic("sysread: unknown file type\n");
    return -1;
}
int sys_spawn(char *filename) {
    return spawn(filename);
}
uint64
sys_pipe(uint64 fdarray) {
    infof("init pipe\n");
    struct proc *p = curr_proc();
    uint64 fd0, fd1;
    struct file *f0, *f1;
    if (f0 < 0 || f1 < 0) {
        return -1;
    }
    f0 = filealloc();
    f1 = filealloc();
    if (pipealloc(f0, f1) < 0)
        return -1;
    fd0 = fdalloc(f0);
    fd1 = fdalloc(f1);
    if (copyout(p->pagetable, fdarray, (char *)&fd0, sizeof(fd0)) < 0 ||
        copyout(p->pagetable, fdarray + sizeof(uint64), (char *)&fd1, sizeof(fd1)) < 0) {
        fileclose(f0);
        fileclose(f1);
        p->files[fd0] = 0;
        p->files[fd1] = 0;
        return -1;
    }
    return 0;
}

uint64 sys_exit(int code) {
    infof("sys_exit");
    exit(code);
    return 0;
}

uint64 sys_sched_yield() {
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

uint64 sys_exec(uint64 va, const char** argv_va) {
    struct proc *p = curr_proc();
    char name[200];
    char argv_str[MAX_EXEC_ARG_COUNT][MAX_EXEC_ARG_LENGTH];
    copyinstr(p->pagetable, name, va, 200);
    infof("sys_exec %s", name);

    int argc = 0;
    const char *argv[MAX_EXEC_ARG_COUNT];
    // tracecore("argv_va=%d argc=%d", argv_va, argc);
    if (argv_va == NULL) {
        // nothing
    } else {
        const char **argv_pa = (const char **)virt_addr_to_physical(p->pagetable, (uint64)argv_va);
        if(argv_pa == NULL){
            // invalid argv_va
            // TODO: kill
            return -1;
        }
        while(*argv_pa){
            const char *argv_one_va = *argv_pa;

            if (copyinstr(p->pagetable, argv_str[argc], (uint64)argv_one_va, MAX_EXEC_ARG_LENGTH) < 0) {
                // TODO: failed, exit
                return -1;
            }
            argv[argc] = argv_str[argc];
            argc++;
            argv_pa++;
        }
    }
    tracecore("argv_va=%d argc=%d", argv_va, argc);

    return exec(name, argc, argv);
}

uint64 sys_wait(int pid, uint64 va) {
    struct proc *p = curr_proc();
    int *code = (int *)virt_addr_to_physical(p->pagetable, va);
    return wait(pid, code);
}

uint64 sys_times() {
    return get_time_ms();
}

int64 sys_setpriority(int64 priority) {
    if (2 <= priority) {
        struct proc *cur_pcb = curr_proc();
        cur_pcb->priority = priority;
        return priority;
    }
    return -1;
}

int64 sys_gettimeofday(uint64 *timeval, int tz) {
    // timeval[0] -- sec
    // timeval[0] -- usec
    uint64 us = get_time_us();
    uint64 timeval_ker[2];
    timeval_ker[0] = us / 1000000;
    timeval_ker[1] = us % 1000000;
    copyout(curr_proc()->pagetable, (uint64)timeval, (char *)timeval_ker, sizeof(timeval_ker));
    return 0;
}
uint64 sys_close(int fd) {
    if (fd == STDIN || fd == STDOUT || fd == STDERR)
        return 0;
    struct proc *p = curr_proc();
    struct file *f = p->files[fd];
    // if(f->type != FD_PIPE) {
    //     errorf("unknown file type %d\n", f->type);
    //     panic("fileclose: unknown file type\n");
    // }
    fileclose(f);
    p->files[fd] = NULL;
    return 0;
}

uint64 sys_openat(uint64 va, uint64 omode, uint64 _flags) {
    debugcore("sys_openat");
    // print_cpu(mycpu());
    struct proc *p = curr_proc();
    char path[200];
    copyinstr(p->pagetable, path, va, 200);
    return fileopen(path, omode);
}

int64 sys_mmap(void *start, uint64 len, int prot) {
    if (len == 0)
        return 0;

    if (len > 1024 * 1024 * 1024) {
        return -1;
    }
    uint64 aligned_len = PGROUNDUP(len);

    uint64 offset = (uint64)start & 0xfff;
    if (offset != 0) {
        return -1;
    }
    if ((prot & ~0x7) != 0) {
        return -1;
    }
    if ((prot & 0x7) == 0) {
        return -1;
    }
    struct proc *curr_pcb = curr_proc();
    uint64 map_size = 0;
    while (aligned_len > 0) {
        void *pa = alloc_physical_page();
        // int PER_R = prot & 1;
        // int PER_W = prot & 2;
        // int PER_X = prot & 4;

        if (map1page(curr_pcb->pagetable, (uint64)start,
                     (uint64)pa, PTE_U | (prot << 1)) < 0) {
            debugf("sys_mmap mappages fail\n");
            return -1;
        }
        aligned_len -= PGSIZE;
        start += PGSIZE;
        map_size += PGSIZE;
    }

    if (aligned_len != 0) {
        panic("aligned_len != 0");
    }
    // TODO: add size to proc.size
    debugf("map_size=%p\n", map_size);
    return map_size;
}

int64 sys_munmap(void *start, uint64 len) {
    uint64 va = (uint64)start;
    uint64 a;
    pte_t *pte;
    pagetable_t pagetable = curr_proc()->pagetable;

    if (((uint64)start % PGSIZE) != 0) {
        return -1;
    }
    int npages = PGROUNDUP(len) / PGSIZE;

    for (a = va; a < va + npages * PGSIZE; a += PGSIZE) {
        if ((pte = walk(pagetable, a, FALSE)) == 0) {
            infof("uvmunmap: walk\n");
            return -1;
        }
        if ((*pte & PTE_V) == 0) {
            infof("uvmunmap: not mapped\n");
            return -1;
        }
        if (PTE_FLAGS(*pte) == PTE_V) {
            infof("uvmunmap: not a leaf\n");
            return -1;
        }

        uint64 pa = PTE2PA(*pte);
        recycle_physical_page((void *)pa);

        *pte = 0;
    }
    return npages * PGSIZE;
}
int sys_mailread(void *buf, int len) {
    infof("mailread\n");
    if (len > 256) {
        len = 256;
    }
    if (len < 0) {
        return -1;
    }
    struct proc *p = curr_proc();

    struct mailbox *inbox = &(p->mb);

    acquire(&inbox->lock);
    if (len == 0) {
        for (int i = 0; i < MAX_MAIL_IN_BOX; i++) {
            if (inbox->valid[i]) {
                release(&inbox->lock);
                return 0;
            }
        }
        release(&inbox->lock);
        return -1;
    }

    // read head mail
    int head_idx = inbox->head;
    if (inbox->valid[head_idx]) {
        int msg_len = inbox->length[head_idx];
        int copy_len = min(msg_len, len);
        int eret = copyout(p->pagetable, (uint64)buf, inbox->mailbuf[head_idx], copy_len);
        if (eret < 0) {
            infof("copyout failed\n");
            release(&inbox->lock);
            return -1;
        }
        inbox->valid[inbox->head] = 0;
        inbox->head += 1;
        inbox->head = (inbox->head) % MAX_MAIL_IN_BOX;
        release(&inbox->lock);
        infof("read mail %d bytes\n", copy_len);
        return copy_len;
    } else {
        // mail box is empty
        release(&inbox->lock);
        infof("mail box is empty\n");
        return -1;
    }
}

int sys_mailwrite(int pid, void *buf, int len) {
    infof("mailwrite\n");
    if (len > 256) {
        len = 256;
    }
    if (len < 0) {
        return -1;
    }
    struct proc *cur_p = curr_proc();

    struct proc *p = findproc(pid);
    if (p == 0) {
        return -1;
    }
    struct mailbox *dest = &(p->mb);

    acquire(&dest->lock);

    if (len == 0) {
        for (int i = 0; i < MAX_MAIL_IN_BOX; i++) {
            if (!(dest->valid[i])) {
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
    for (int j = 0; j < MAX_MAIL_IN_BOX; j++) {
        if (dest->valid[j] != 0) {
            // not empty, find next
        } else {
            // empty, write to this one
            int eret = copyin(cur_p->pagetable, dest->mailbuf[j], (uint64)buf, len);
            if (eret < 0) {
                infof("copyin failed\n");
                release(&dest->lock);
                return -1;
            }
            dest->valid[j] = 1;
            dest->length[j] = len;
            release(&dest->lock);
            return len;
        }
        head_idx += 1;
        head_idx = head_idx % MAX_MAIL_IN_BOX;
    }

    // all filled
    release(&dest->lock);
    return -1;
}

uint64 sys_write(int fd, uint64 va, uint64 len) {
    if (fd == 1) {
        return console_write(va, len);
    }
    if (fd >= FD_MAX) {
        return -1;
    }
    infof("fd=%d\n", fd);
    struct proc *p = curr_proc();
    struct file *f = p->files[fd];
    if (f == 0) {
        return -1;
    }
    if (f->type == FD_PIPE) {
        debugf("write to pipe at %p\n", f->pipe);
        return pipewrite(f->pipe, va, len);
    } else if (f->type == FD_INODE) {
        return filewrite(f, va, len);
    }
    errorf("unknown file type %d\n", f->type);
    panic("syswrite: unknown file type\n");
    return -1;
}
