#ifndef __FILE_H__
#define __FILE_H__

#include "types.h"
#include "lock.h"
#include "fs.h"
// pipe.h
#define PIPESIZE 512
// in-memory copy of an inode
struct inode
{
    uint dev;   // Device number
    uint inum;  // Inode number
    int ref;    // Reference count
    int valid;  // inode has been read from disk?
    short type; // copy of disk inode
    uint size;
    uint addrs[NDIRECT + 1];
};

// map major device number to device functions.
struct devsw
{
    int (*read)(int, uint64, int);
    int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

struct pipe
{
    char data[PIPESIZE];
    uint nread;    // number of bytes read
    uint nwrite;   // number of bytes written
    int readopen;  // read fd is still open
    int writeopen; // write fd is still open
    struct spinlock lock;
};

// file.h
struct file
{
    enum
    {
        FD_NONE = 0,
        FD_PIPE,
        FD_INODE
    } type;

    int ref; // reference count
    char readable;
    char writable;
    struct pipe *pipe; // FD_PIPE
    struct inode *ip;  // FD_INODE
    uint off;          // FD_INODE
};

typedef char mail_t[256];
#define MAX_MAIL_IN_BOX (16)
struct mailbox
{
    mail_t *mailbuf; // 4KB, 16 mail
    int valid[MAX_MAIL_IN_BOX];
    int length[MAX_MAIL_IN_BOX];
    int head;
    struct spinlock lock;
};

int init_mailbox(struct mailbox *mb);

extern struct file filepool[128 * 16]; // NPROC * PROC_MAX

#endif //!__FILE_H__