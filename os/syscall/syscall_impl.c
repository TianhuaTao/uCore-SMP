#include "syscall_impl.h"
#include <arch/timer.h>
#include <file/file.h>
#include <proc/proc.h>
#define min(a, b) (a) < (b) ? (a) : (b);

// int sys_spawn(char *filename) {
//     return spawn(filename);
// }
int sys_pipe(int (*pipefd_va)[2]) {
    struct proc *p = curr_proc();
    struct file *rf, *wf;
    int fd0, fd1;
    int(*pipefd)[2];
    pipefd = (void *)virt_addr_to_physical(p->pagetable, (uint64)pipefd_va);
    if (pipefd == NULL) {
        infof("pipefd invalid");
        return -1;
    }
    if (pipealloc(&rf, &wf) < 0) {
        return -1;
    }
    fd0 = -1;
    if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) {
        if (fd0 >= 0)
            p->files[fd0] = 0;
        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    infof("fd0=%d, fd1=%d", fd0, fd1);
    phex(pipefd_va);
    if (copyout(p->pagetable, (uint64)pipefd_va, (char *)&fd0, sizeof(fd0)) < 0 ||
        copyout(p->pagetable, (uint64)pipefd_va + sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0) {
        p->files[fd0] = 0;
        p->files[fd1] = 0;
        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    return 0;
}

uint64 sys_exit(int code) {
    exit(code);
    return 0;
}

uint64 sys_sched_yield() {
    yield();
    return 0;
}

uint64 sys_getpid() {
    return curr_proc()->pid;
}

uint64 sys_clone() {
    return fork();
}

/**
 * @brief Create directory at given path
 * 
 * @param path_va Points to the path at user space
 * @return int64 0 if successfull, otherwise failed 
 */
int64 sys_mkdir(char *path_va) {
    struct proc *p = curr_proc();
    char path[MAXPATH];
    struct inode *ip;

    if (copyinstr(p->pagetable, path, (uint64)path_va, MAXPATH) != 0) {
        return -2;
    }
    ip = create(path, T_DIR, 0, 0);

    if (ip == NULL) {
        return -1;
    }
    iunlockput(ip);

    return 0;
}

int64 sys_chdir(char *path_va) {
    char path[MAXPATH];
    struct inode *ip;
    struct proc *p = curr_proc();

    if (copyinstr(p->pagetable, path, (uint64)path_va, MAXPATH) != 0) {
        return -2;
    }
    ip = namei(path);
    if (ip == NULL) {
        return -1;
    }
    ilock(ip);
    if (ip->type != T_DIR) {
        iunlockput(ip);
        return -1;
    }
    iunlock(ip);
    iput(p->cwd);

    p->cwd = ip;
    return 0;
}

int sys_mknod(char *path_va, short major, short minor) {
    struct proc *p = curr_proc();
    struct inode *ip;
    char path[MAXPATH];

    if (copyinstr(p->pagetable, path, (uint64)path_va, MAXPATH) != 0) {
        debugcore("can not copyinstr");
        return -1;
    }
    ip = create(path, T_DEVICE, major, minor);

    if (ip == NULL) {
        debugcore("can not create inode");
        return -1;
    }

    iunlockput(ip);

    return 0;
}

uint64 sys_exec(uint64 va, const char **argv_va) {
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
        if (argv_pa == NULL) {
            // invalid argv_va
            // TODO: kill
            return -1;
        }
        while (*argv_pa) {
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

int sys_wait(int pid, uint64 exitcode_va) {
    struct proc *p = curr_proc();
    int *code = (int *)virt_addr_to_physical(p->pagetable, exitcode_va);
    return wait(pid, code);
}

uint64 sys_times() {
    return get_time_ms();
}

/**
 * @brief Set priority of current process
 * 
 * @param priority >=2
 * @return int64 return the priority set, or -1 if failed
 */
int64 sys_setpriority(int64 priority) {
    if (2 <= priority) {
        struct proc *p = curr_proc();
        acquire(&p->lock);
        p->priority = priority;
        release(&p->lock);
        return priority;
    }
    return -1;
}

/**
 * @brief Get priority of current process
 * 
 * @return int64 priority
 */
int64 sys_getpriority() {
    int64 priority;
    struct proc *p = curr_proc();
    acquire(&p->lock);
    priority = p->priority;
    release(&p->lock);
    return priority;
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

/**
 * @brief Close a file
 * 
 * @param fd file descriptor
 * @return uint64 0 if successful, nonzero if not
 */
uint64 sys_close(int fd) {
    // TODO: validate fd
    struct proc *p = curr_proc();

    // invalid fd
    if (fd < 0 || fd >= FD_MAX) {
        infof("invalid fd %d", fd);
        return -1;
    }

    struct file *f = p->files[fd];

    // invalid fd
    if (f == NULL) {
        infof("fd %d is not opened", fd);
        return -1;
    }

    p->files[fd] = NULL;

    fileclose(f);
    return 0;
}

int sys_open(uint64 path_va, int flags) {
    // TODO: rewrite
    debugcore("sys_open");
    struct proc *p = curr_proc();
    char path[MAXPATH];
    copyinstr(p->pagetable, path, path_va, MAXPATH);
    return fileopen(path, flags);
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
// int sys_mailread(void *buf, int len) {
//     infof("mailread\n");
//     if (len > 256) {
//         len = 256;
//     }
//     if (len < 0) {
//         return -1;
//     }
//     struct proc *p = curr_proc();

//     struct mailbox *inbox = &(p->mb);

//     acquire(&inbox->lock);
//     if (len == 0) {
//         for (int i = 0; i < MAX_MAIL_IN_BOX; i++) {
//             if (inbox->valid[i]) {
//                 release(&inbox->lock);
//                 return 0;
//             }
//         }
//         release(&inbox->lock);
//         return -1;
//     }

//     // read head mail
//     int head_idx = inbox->head;
//     if (inbox->valid[head_idx]) {
//         int msg_len = inbox->length[head_idx];
//         int copy_len = min(msg_len, len);
//         int eret = copyout(p->pagetable, (uint64)buf, inbox->mailbuf[head_idx], copy_len);
//         if (eret < 0) {
//             infof("copyout failed\n");
//             release(&inbox->lock);
//             return -1;
//         }
//         inbox->valid[inbox->head] = 0;
//         inbox->head += 1;
//         inbox->head = (inbox->head) % MAX_MAIL_IN_BOX;
//         release(&inbox->lock);
//         infof("read mail %d bytes\n", copy_len);
//         return copy_len;
//     } else {
//         // mail box is empty
//         release(&inbox->lock);
//         infof("mail box is empty\n");
//         return -1;
//     }
// }

// int sys_mailwrite(int pid, void *buf, int len) {
//     infof("mailwrite\n");
//     if (len > 256) {
//         len = 256;
//     }
//     if (len < 0) {
//         return -1;
//     }
//     struct proc *cur_p = curr_proc();

//     struct proc *p = findproc(pid);
//     if (p == 0) {
//         return -1;
//     }
//     struct mailbox *dest = &(p->mb);

//     acquire(&dest->lock);

//     if (len == 0) {
//         for (int i = 0; i < MAX_MAIL_IN_BOX; i++) {
//             if (!(dest->valid[i])) {
//                 // empty slot
//                 release(&dest->lock);
//                 return 0;
//             }
//         }
//         release(&dest->lock);
//         return -1;
//     }

//     // write mail
//     int head_idx = dest->head;
//     for (int j = 0; j < MAX_MAIL_IN_BOX; j++) {
//         if (dest->valid[j] != 0) {
//             // not empty, find next
//         } else {
//             // empty, write to this one
//             int eret = copyin(cur_p->pagetable, dest->mailbuf[j], (uint64)buf, len);
//             if (eret < 0) {
//                 infof("copyin failed\n");
//                 release(&dest->lock);
//                 return -1;
//             }
//             dest->valid[j] = 1;
//             dest->length[j] = len;
//             release(&dest->lock);
//             return len;
//         }
//         head_idx += 1;
//         head_idx = head_idx % MAX_MAIL_IN_BOX;
//     }

//     // all filled
//     release(&dest->lock);
//     return -1;
// }
ssize_t sys_read(int fd, void *dst_va, size_t len) {
    if (fd >= FD_MAX || fd < 0) {
        return -1;
    }
    struct proc *p = curr_proc();
    struct file *f = p->files[fd];
    if (f == NULL) {
        return -1;
    }
    return fileread(f, dst_va, len);
}

ssize_t sys_write(int fd, void *src_va, size_t len) {
    if (fd >= FD_MAX || fd < 0) {
        return -1;
    }
    struct proc *p = curr_proc();
    struct file *f = p->files[fd];
    if (f == NULL) {
        return -1;
    }

    return filewrite(f, src_va, len);
}

int sys_dup(int oldfd) {
    struct file *f;
    int fd;
    struct proc *p = curr_proc();
    f = get_proc_file_by_fd(p, oldfd);

    if (f == NULL) {
        infof("old fd is not valid");
        print_proc(p);
        return -1;
    }

    if ((fd = fdalloc(f)) < 0) {
        infof("cannot allocate new fd");
        return -1;
    }
    filedup(f);
    return fd;
}

// Create the path new as a link to the same inode as old.
int link(char *oldpath_va, char *newpath_va) {
    char name[DIRSIZ], new[MAXPATH], old[MAXPATH];
    struct inode *dp, *ip;

    struct proc *p = curr_proc();
    if (copyinstr(p->pagetable, old, (uint64)oldpath_va, MAXPATH) != 0) {
        return -1;
    }
    if (copyinstr(p->pagetable, new, (uint64)newpath_va, MAXPATH) != 0) {
        return -1;
    }

    if ((ip = namei(old)) == 0) {
        return -1;
    }

    ilock(ip);
    if (ip->type == T_DIR) {
        iunlockput(ip);
        return -1;
    }

    ip->num_link++;
    iupdate(ip);
    iunlock(ip);

    if ((dp = nameiparent(new, name)) == 0)
        goto bad;
    ilock(dp);
    if (dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0) {
        iunlockput(dp);
        goto bad;
    }
    iunlockput(dp);
    iput(ip);

    return 0;

bad:
    ilock(ip);
    ip->num_link--;
    iupdate(ip);
    iunlockput(ip);

    return -1;
}

int unlink(char *pathname_va) {
    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ], path[MAXPATH];
    uint off;

    struct proc *p = curr_proc();
    if (copyinstr(p->pagetable, path, (uint64)pathname_va, MAXPATH) != 0) {
        return -1;
    }

    if ((dp = nameiparent(path, name)) == 0) {
        return -1;
    }

    ilock(dp);

    // Cannot unlink "." or "..".
    if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
        goto bad;

    if ((ip = dirlookup(dp, name, &off)) == 0)
        goto bad;
    ilock(ip);

    if (ip->num_link < 1)
        panic("unlink: nlink < 1");
    if (ip->type == T_DIR && !isdirempty(ip)) {
        iunlockput(ip);
        goto bad;
    }

    memset(&de, 0, sizeof(de));
    if (writei(dp, 0, &de, off, sizeof(de)) != sizeof(de))
        panic("unlink: writei");
    if (ip->type == T_DIR) {
        dp->num_link--;
        iupdate(dp);
    }
    iunlockput(dp);

    ip->num_link--;
    iupdate(ip);
    iunlockput(ip);
    return 0;

bad:
    iunlockput(dp);
    return -1;
}