#if !defined(BUF_H)
#define BUF_H

#include "fs.h"
#include "types.h"
struct buf
{
    int valid; // has data been read from disk?
    int disk;  // does disk "own" buf?
    uint dev;
    uint blockno;
    uint refcnt;
    struct buf *prev; // LRU cache list
    struct buf *next;
    uchar data[BSIZE];
};

#endif // BUF_H
