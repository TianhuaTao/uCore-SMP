#include "fcntl.h"
#include <file/file.h>
#include <fs/fs.h>
#include <proc/proc.h>
#include <ucore/defs.h>
#include <ucore/types.h>
#include <device/console.h>
#include <file/stat.h>
#include "stdafx.h"
#include <stdio.h>
/**
 * @brief The global file pool
 * Every opened file is kept here in system level
 * Process files are pointing here.
 * 
 */
struct {
    struct file files[FILE_MAX];    // system level files
    struct spinlock lock;
} filepool;
struct device_rw_handler device_rw_handler[NDEV];
void console_init();
void cpu_device_init();
void mem_device_init();
void proc_device_init();

/**
 * @brief Call xxx_init of all devices
 * 
 */
void device_init() {
    console_init();
    cpu_device_init();
    mem_device_init();
    proc_device_init();
}
/**
 * @brief Init the global file pool
 * 
 */
void fileinit() {
    init_spin_lock_with_name(&filepool.lock, "filepool.lock");
    device_init();
}

/**
 * @brief Release a reference to a file, close the file if ref is zero
 * 
 * @param f the file in global file pool
 */
void fileclose(struct file *f) {
    struct file ff;
    acquire(&filepool.lock);
    KERNEL_ASSERT(f->ref >= 1, "file reference should be at least 1");
    --f->ref;
    if (f->ref > 0) {
        // some other process is using it
        release(&filepool.lock);
        return;
    }

    // clear the file
    ff = *f;
    f->ref = 0;
    f->type = FD_NONE;
    release(&filepool.lock);

    if (ff.type == FD_PIPE) {
        pipeclose(ff.pipe, ff.writable);
    } else if (ff.type == FD_INODE || ff.type == FD_DEVICE) {
        iput(ff.ip);
    }
}

/**
 * @brief Scans the file table for an unreferenced file
 * 
 * @return struct file* the unreferenced file, or NULL if all used
 */
struct file *filealloc() {
    acquire(&filepool.lock);
    for (int i = 0; i < FILE_MAX; ++i) {
        if (filepool.files[i].ref == 0) {
            filepool.files[i].ref = 1;
            release(&filepool.lock);
            return &filepool.files[i];
        }
    }
    release(&filepool.lock);
    return NULL;
}

struct inode * create(char *path, short type, short major, short minor) {
    struct inode *ip, *dp;
    char name[DIRSIZ];

    if ((dp = inode_parent_by_name(path, name)) == 0)
        return 0;

    ilock(dp);
    uint devnum=dp->dev;
    if ((ip = dirlookup(devnum,dp,name,path)) != 0) {
        iunlockput(dp);
        ilock(ip);
        if (type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
            return ip;
        iunlockput(ip);
        return 0;
    }
    //don't find then create
    else{
        
    }
    // if ((ip = alloc_disk_inode(dp->dev, type)) == 0)
    //     panic("create: ialloc");

    ilock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->num_link = 1;
    iupdate(ip);

    if (type == T_DIR) { // Create . and .. entries.
        dp->num_link++;  // for ".."
        iupdate(dp);
        // No ip->nlink++ for ".": avoid cyclic ref count.
        if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
            panic("create dots");
    }

    if (dirlink(dp, name, ip->inum) < 0)
        panic("create: dirlink");

    iunlockput(dp);

    return ip;
}

/**
 * @brief Increment ref count for file f
 * 
 * @param f the file
 * @return struct file* return f itselt
 */
struct file *
filedup(struct file *f) {
    acquire(&filepool.lock);
    KERNEL_ASSERT(f->ref >= 1, "file reference should be at least 1");
    f->ref++;
    release(&filepool.lock);
    return f;
}

/**
 * @brief Open a file
 * 
 * @param path kernel space string
 * @param flags how to open
 * @return int fd, -1 if failed
 */
int fileopen(char *path, int flags) {
    debugcore("fileopen");
    int fd;
    struct file *f;
    struct inode *ip;


    if (flags & O_CREATE) {
        ip = create(path, T_FILE, 0, 0);
        if (ip == NULL) {
            infof("Cannot create inode");
            return -1;
        }
    } else {
        // find inode by name
        if ((ip = inode_by_name(path)) == NULL) {
            infof("Cannot find inode with name %s", path);
            return -1;
        }
        // the inode is found
        ilock(ip);
        if (ip->type == T_DIR && flags != O_RDONLY) {
            iunlockput(ip);
            infof("Can only read a dir");
            return -1;
        }
    }

    if (ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)) {
        iunlockput(ip);
        return -1;
    }

    if ((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0) {
        if (f)
            fileclose(f);
        iunlockput(ip);
        return -1;
    }

    if (ip->type == T_DEVICE) {
        f->type = FD_DEVICE;
        f->major = ip->major;
    } else {
        f->type = FD_INODE;
        f->off = 0;
    }
    f->ip = ip;
    f->readable = !(flags & O_WRONLY);
    f->writable = (flags & O_WRONLY) || (flags & O_RDWR);

    if ((flags & O_TRUNC) && ip->type == T_FILE) {
        itrunc(ip);
    }

    iunlock(ip);
    return fd;
}

ssize_t filewrite(struct file *f, void* src_va, size_t len) {
    int r, ret = 0;

    if (f->writable == 0)
        return -1;

    if (f->type == FD_PIPE) {
        ret = pipewrite(f->pipe, (uint64)src_va, len);
    } else if (f->type == FD_DEVICE) {
        if (f->major < 0 || f->major >= NDEV || !device_rw_handler[f->major].write)
            return -1;
        ret = device_rw_handler[f->major].write((char*)src_va, len, TRUE);
    } else if (f->type == FD_INODE) {
        // write a few blocks at a time to avoid exceeding
        // the maximum log transaction size, including
        // i-node, indirect block, allocation blocks,
        // and 2 blocks of slop for non-aligned writes.
        // this really belongs lower down, since writei()
        // might be writing a device like the console.
        int max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * BSIZE;
        int i = 0;
        while (i < len) {
            int n1 = len - i;
            if (n1 > max)
                n1 = max;

            ilock(f->ip);
            if ((r = writei(f->ip, 1, src_va + i, f->off, n1)) > 0)
                f->off += r;
            iunlock(f->ip);

            if (r != n1) {
                // error from writei
                break;
            }
            i += r;
        }
        ret = (i == len ? len : -1);
    } else {
        panic("filewrite");
    }

    return ret;
}

// Read from file f.
// addr is a user virtual address.
ssize_t fileread(struct file *f, void* dst_va, size_t len) {
    int r = 0;

    if (f->readable == 0)
        return -1;

    if (f->type == FD_PIPE) {
        r = piperead(f->pipe, (uint64)dst_va, len);
    } else if (f->type == FD_DEVICE) {
        if (f->major < 0 || f->major >= NDEV || !device_rw_handler[f->major].read)
            return -1;
        r = device_rw_handler[f->major].read( dst_va, len, TRUE);
    } else if (f->type == FD_INODE) {
        ilock(f->ip);
        if ((r = readi(f->ip, TRUE, dst_va, f->off, len)) > 0)
            f->off += r;
        iunlock(f->ip);
    } else {
        panic("fileread");
    }

    return r;
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int filestat(struct file *f, uint64 addr) {
    struct proc *p = curr_proc();
    struct stat st;

    if (f->type == FD_INODE || f->type == FD_DEVICE) {
        ilock(f->ip);
        stati(f->ip, &st);
        iunlock(f->ip);
        if (copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
            return -1;
        return 0;
    }
    return -1;
}


// int init_mailbox(struct mailbox *mb) {
//     void *buf_pa = alloc_physical_page();
//     if (buf_pa == 0) {
//         return 0;
//     }
//     init_spin_lock_with_name(&mb->lock, "mailbox.lock");
//     mb->mailbuf = buf_pa;
//     for (int i = 0; i < MAX_MAIL_IN_BOX; i++) {
//         mb->length[i] = 0;
//         mb->valid[i] = 0;
//     }
//     mb->head = 0;
//     return 1;
// }
