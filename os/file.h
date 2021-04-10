#ifndef __FILE_H__
#define __FILE_H__

#include "types.h"
#include "lock.h"
// pipe.h
#define PIPESIZE 512

struct pipe {
    char data[PIPESIZE];
    uint nread;     // number of bytes read
    uint nwrite;    // number of bytes written
    int readopen;   // read fd is still open
    int writeopen;  // write fd is still open
    struct spinlock lock;
};

// file.h
struct file {
    enum { FD_NONE = 0, FD_PIPE} type;
    int ref; // reference count
    char readable;
    char writable;
    struct pipe *pipe; // FD_PIPE
    uint off;          // FD_INODE
};

typedef char mail_t[256];
# define MAX_MAIL_IN_BOX (16)
struct mailbox{
    mail_t* mailbuf;    // 4KB, 16 mail
    int valid[MAX_MAIL_IN_BOX];
    int length[MAX_MAIL_IN_BOX];
    int head;
    struct spinlock lock;
};

int init_mailbox(struct mailbox* mb);

extern struct file filepool[128 * 16];

#endif //!__FILE_H__