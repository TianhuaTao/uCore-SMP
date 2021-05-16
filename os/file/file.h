#ifndef __FILE_H__
#define __FILE_H__

#include <fs/fs.h>
#include <lock/lock.h>
#include <ucore/types.h>
// pipe.h
#define PIPESIZE 512
// in-memory copy of an inode
#define major(dev) ((dev) >> 16 & 0xFFFF)
#define minor(dev) ((dev)&0xFFFF)
#define mkdev(m, n) ((uint)((m) << 16 | (n)))
struct inode {
    uint dev;  // Device number
    uint inum; // Inode number
    int ref;   // Reference count
    struct mutex lock;
    int valid;  // inode has been read from disk?

    // disk side information, only valid if this->valid == true
    short type; 
    short major;
    short minor;
    short num_link;
    uint size;
    uint addrs[NDIRECT + 1];
};

// map major device number to device functions.
struct devsw {
    int64 (*read)(char *dst, int64 len, int to_user);
    int64 (*write)(char *src, int64 len, int from_user);
};

extern struct devsw devsw[];

struct pipe {
    char data[PIPESIZE];
    uint nread;    // number of bytes read
    uint nwrite;   // number of bytes written
    int readopen;  // read fd is still open
    int writeopen; // write fd is still open
    struct spinlock lock;
};

// file.h
struct file {
    enum {
        FD_NONE = 0,
        FD_PIPE,
        FD_INODE,
        FD_DEVICE
    } type;

    int ref; // reference count
    char readable;
    char writable;
    struct pipe *pipe; // FD_PIPE
    struct inode *ip;  // FD_INODE
    uint off;          // FD_INODE
    short major;       // FD_DEVICE
};

// typedef char mail_t[256];
// #define MAX_MAIL_IN_BOX (16)
// struct mailbox {
//     mail_t *mailbuf; // 4KB, 16 mail
//     int valid[MAX_MAIL_IN_BOX];
//     int length[MAX_MAIL_IN_BOX];
//     int head;
//     struct spinlock lock;
// };

// int init_mailbox(struct mailbox *mb);
struct inode *create(char *path, short type, short major, short minor);
void fileinit();
void device_init();
void fileclose(struct file *f);
ssize_t fileread(struct file *f, void *dst_va, size_t len);
ssize_t filewrite(struct file *f, void *src_va, size_t len);
struct file *filealloc();
int fileopen(char *path, int flags);
struct file *filedup(struct file *f);
int filestat(struct file *f, uint64 addr);
#define FILE_MAX (128 * 16)

#define CONSOLE 1

#endif //!__FILE_H__