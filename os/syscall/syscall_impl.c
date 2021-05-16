#include "syscall_impl.h"
#include <arch/timer.h>
#include <file/file.h>
#include <proc/proc.h>
#include <file/stat.h>
#define min(a, b) (a) < (b) ? (a) : (b);

int sys_fstat(int fd, struct stat *statbuf_va){
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

    return filestat(f, (uint64)statbuf_va);

}

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

int sys_exit(int code) {
    exit(code);
    return 0;
}

int sys_sched_yield() {
    yield();
    return 0;
}

pid_t sys_getpid() {
    return curr_proc()->pid;
}

pid_t sys_getppid() {
    // TODO: use lock
    struct proc* p = curr_proc();
    struct proc* parent = p->parent;
    int ppid;
    if(parent){
        ppid = parent->pid;
        return ppid;
    }
    return -1;
}

pid_t sys_fork() {
    return fork();
}

/**
 * @brief Create directory at given path
 *
 * @param path_va Points to the path at user space
 * @return int64 0 if successfull, otherwise failed
 */
int sys_mkdir(char *path_va) {
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

int sys_chdir(char *path_va) {
    char path[MAXPATH];
    struct inode *ip;
    struct proc *p = curr_proc();

    if (copyinstr(p->pagetable, path, (uint64)path_va, MAXPATH) != 0) {
        return -2;
    }
    ip = inode_by_name(path);
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

int sys_execv( char *pathname_va, char * argv_va[]) {
    struct proc *p = curr_proc();
    char name[MAXPATH];
    char argv_str[MAX_EXEC_ARG_COUNT][MAX_EXEC_ARG_LENGTH];
    copyinstr(p->pagetable, name, (uint64)pathname_va, MAXPATH);
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

pid_t sys_waitpid(pid_t pid, int *wstatus_va) {
    struct proc *p = curr_proc();
    // TODO: bug
    // TODO: use copyout
    int *code = (int *)virt_addr_to_physical(p->pagetable, (uint64)wstatus_va);
    return wait(pid, code);
}

uint64 sys_time_ms() {
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


int sys_close(int fd) {
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

int sys_open( char *pathname_va, int flags){
    debugcore("sys_open");
    struct proc *p = curr_proc();
    char path[MAXPATH];
    copyinstr(p->pagetable, path, (uint64)pathname_va, MAXPATH);
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
int sys_link(char *oldpath_va, char *newpath_va) {
    char name[DIRSIZ], new[MAXPATH], old[MAXPATH];
    struct inode *dp, *ip;

    struct proc *p = curr_proc();
    if (copyinstr(p->pagetable, old, (uint64)oldpath_va, MAXPATH) != 0) {
        return -1;
    }
    if (copyinstr(p->pagetable, new, (uint64)newpath_va, MAXPATH) != 0) {
        return -1;
    }

    if ((ip = inode_by_name(old)) == 0) {
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

    if ((dp = inode_parent_by_name(new, name)) == 0)
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

int sys_unlink(char *pathname_va) {
    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ], path[MAXPATH];
    uint off;

    struct proc *p = curr_proc();
    if (copyinstr(p->pagetable, path, (uint64)pathname_va, MAXPATH) != 0) {
        return -1;
    }

    if ((dp = inode_parent_by_name(path, name)) == 0) {
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

int sys_set_dsid(int pid, uint32 dsid)
{
    struct proc *p = findproc(pid);
    if (p == 0)
    {
        warnf("pid %d not found!", pid);
        return -1;
    }
    acquire(&p->lock);
    p->dsid = dsid;
    release(&p->lock);
    infof("set pid: %d, dsid: %d", pid, dsid);
    return 0;
}

#include "../dsid/dsid.h"

int sys_set_dsid_param(uint32 dsid, uint32 freq, uint32 size, uint32 inc, uint32 mask)
{
    if (mask > 0xFFFF)
        mask = 0xFFFF;
    acquire(&dsid_lock);
    cp_reg_w(CP_DSID_SEL - CP_HART_DSID, dsid << 2);
    if (freq != 0)
        cp_reg_w(CP_BUCKET_FREQ - CP_HART_DSID, freq);
    if (size != 0)
        cp_reg_w(CP_BUCKET_SIZE - CP_HART_DSID, size);
    if (inc != 0)
        cp_reg_w(CP_BUCKET_INC - CP_HART_DSID, inc);
    if (mask != 0)
        cp_reg_w(CP_WAYMASK - CP_HART_DSID, mask);
    infof("set dsid: %d, freq: %d, size: 0x%x, inc: 0x%x, mask: 0x%x", dsid, cp_reg_r(CP_BUCKET_FREQ - CP_HART_DSID), cp_reg_r(CP_BUCKET_SIZE - CP_HART_DSID), cp_reg_r(CP_BUCKET_INC - CP_HART_DSID), cp_reg_r(CP_WAYMASK - CP_HART_DSID));
    release(&dsid_lock);
    return 0;
}

uint32 sys_get_l2_traffic(uint32 dsid)
{
    acquire(&dsid_lock);
    cp_reg_w(CP_DSID_SEL - CP_HART_DSID, dsid << 2);

    // infof("current pid: %d, core: %d, dsid: %x, freq: %x, size: %x, inc: %x, mask: %x", curr_proc()->pid, r_tp(), r_dsid(), cp_reg_r(CP_BUCKET_FREQ - CP_HART_DSID), cp_reg_r(CP_BUCKET_SIZE - CP_HART_DSID), cp_reg_r(CP_BUCKET_INC - CP_HART_DSID), cp_reg_r(CP_WAYMASK - CP_HART_DSID));
    // infof("all info of dsid %x:", dsid);
    // for (int i = 0, cnt = 0; i < 63; ++i)
    // {
    //     const char *name = cp_reg_name[i];
    //     if (name && name[0] != 'N')
    //     {
    //         printf("%s: 0x%x,  ", name, cp_reg_r(i));
    //         ++cnt;
    //         if (cnt % 5 == 4)
    //             printf("\n");
    //     }
    // }
    // printf("-------\n\n");

    uint32 ret = cp_reg_r(CP_TRAFFIC - CP_HART_DSID);
    release(&dsid_lock);

    return ret;
}
