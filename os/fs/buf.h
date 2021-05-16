#if !defined(BUF_H)
#define BUF_H

#include <fs/fs.h>
#include <ucore/types.h>
#include <lock/lock.h>
struct buf
{
    int valid; // has data been read from disk?
    int disk_is_reading;  // does disk "own" buf?
    uint dev;
    uint blockno;
    struct mutex mu;
    uint refcnt;
    struct buf *prev; // LRU cache list
    struct buf *next;
    uchar data[BSIZE];
};

#endif // BUF_H
