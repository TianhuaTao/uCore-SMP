#include "fcntl.h"
#include <file/file.h>
#include <fs/fs.h>
#include <proc/proc.h>
#include <ucore/defs.h>
#include <ucore/types.h>

struct {
    struct file files[FILE_MAX];
    struct spinlock lock;
} filepool;

void fileinit() {
    init_spin_lock_with_name(&filepool.lock, "filepool.lock");
}

void fileclose(struct file *f) {
    struct file ff;
    acquire(&filepool.lock);
    if (f->ref < 1)
        panic("fileclose");
    if (--f->ref > 0) {
        release(&filepool.lock);
        return;
    }
    ff = *f;
    f->ref = 0;
    f->type = FD_NONE;
    release(&filepool.lock);

    if (ff.type == FD_PIPE) {
        pipeclose(ff.pipe, ff.writable);
    } else if (ff.type == FD_INODE) {
        iput(ff.ip);
    }
}

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
    return 0;
}

extern int PID;

static struct inode *
create(char *path, short type) {
    struct inode *ip, *dp;
    debugcore("create");
    dp = root_dir();
    ivalid(dp);

    if ((ip = dirlookup(dp, path, 0)) != 0) {
        warnf("create a exist file\n");
        iput(dp);
        ivalid(ip);
        if (type == T_FILE && ip->type == T_FILE)
            return ip;
        iput(ip);
        return 0;
    }
    debugcore("dirlookup done");

    if ((ip = ialloc(dp->dev, type)) == 0)
        panic("create: ialloc");

    tracef("create dinod and inode type = %d\n", type);

    ivalid(ip);

    iupdate(ip);

    if (dirlink(dp, path, ip->inum) < 0)
        panic("create: dirlink");

    iput(dp);
    return ip;
}

extern int PID;

int fileopen(char *path, uint64 omode) {
    debugcore("fileopen");
    int fd;
    struct file *f;
    struct inode *ip;

    if (omode & O_CREATE) {
        debugcore("create ");
        ip = create(path, T_FILE);
        debugcore("create done");

        if (ip == NULL) {
            return -1;
        }
    } else {
        if ((ip = namei(path)) == 0) {
            return -1;
        }
        ivalid(ip);
    }
    if (ip->type != T_FILE)
        panic("unsupported file inode type\n");
    if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0) {
        if (f)
            fileclose(f);
        iput(ip);
        return -1;
    }
    // only support FD_INODE
    f->type = FD_INODE;
    f->off = 0;
    f->ip = ip;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
    if ((omode & O_TRUNC) && ip->type == T_FILE) {
        itrunc(ip);
    }
    return fd;
}

uint64 filewrite(struct file *f, uint64 va, uint64 len) {
    int r;
    ivalid(f->ip);
    if ((r = writei(f->ip, 1, va, f->off, len)) > 0)
        f->off += r;
    return r;
}

uint64 fileread(struct file *f, uint64 va, uint64 len) {
    int r;
    ivalid(f->ip);
    if ((r = readi(f->ip, 1, va, f->off, len)) > 0)
        f->off += r;
    return r;
}

int init_mailbox(struct mailbox *mb) {
    void *buf_pa = alloc_physical_page();
    if (buf_pa == 0) {
        return 0;
    }
    init_spin_lock_with_name(&mb->lock, "mailbox.lock");
    mb->mailbuf = buf_pa;
    for (int i = 0; i < MAX_MAIL_IN_BOX; i++) {
        mb->length[i] = 0;
        mb->valid[i] = 0;
    }
    mb->head = 0;
    return 1;
}
