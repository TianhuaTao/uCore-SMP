// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call acquire_buf_and_read.
// * After changing buffer data, call write_buf_to_disk to write it to disk.
// * When done with the buffer, call release_buf.
// * Do not use the buffer after calling release_buf.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include <arch/riscv.h>
#include <fs/buf.h>
#include <fs/fs.h>
#include <lock/lock.h>
#include <ucore/defs.h>
#include <ucore/types.h>
#include <ucore/ucore.h>
struct {
    struct spinlock lock;
    struct buf buf[NBUF];
    struct buf head;
} bcache;

void binit(void) {
    struct buf *b;
    init_spin_lock_with_name(&bcache.lock, "bcache.lock");
    // Create linked list of buffers
    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;
    for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        init_mutex(&b->mu);
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
static struct buf *
acquire_buf(uint dev, uint blockno) {
    // tracecore("acquire_buf");
    struct buf *b;
    acquire(&bcache.lock);

    // Is the block already cached?
    for (b = bcache.head.next; b != &bcache.head; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            release(&bcache.lock);
            acquire_mutex_sleep(&b->mu);
            return b;
        }
    }

    // Not cached.
    // Recycle the least recently used (LRU) unused buffer.
    for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
        if (b->refcnt == 0) {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            release(&bcache.lock);
            acquire_mutex_sleep(&b->mu);
            return b;
        }
    }
    panic("acquire_buf: no buffers");
    return 0;
}

const int R = 0;
const int W = 1;

// Return a buf with the contents of the indicated block.
struct buf *
acquire_buf_and_read(uint dev, uint blockno) {
    // debugcore("acquire_buf_and_read");

    struct buf *b;
    b = acquire_buf(dev, blockno);
    // debugcore("acquire_buf ret");

    if (!b->valid) {
        abstract_disk_rw(b, R);
        // virtio_disk_rw(b, R);
        b->valid = 1;
    }
    return b;
}

// Write b's contents to disk.
void write_buf_to_disk(struct buf *b) {
    // tracecore("write_buf_to_disk");
    if (!holdingsleep(&b->mu))
        panic("write_buf_to_disk");
    // virtio_disk_rw(b, W);
    abstract_disk_rw(b,W);
}

// Release a buffer.
// Move to the head of the most-recently-used list.
void release_buf(struct buf *b) {
    // tracecore("release_buf");
    if (!holdingsleep(&b->mu))
        panic("release_buf");

    release_mutex_sleep(&b->mu);
    acquire(&bcache.lock);
    b->refcnt--;
    if (b->refcnt == 0) {
        // no one is waiting for it.
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
    release(&bcache.lock);
}

void bpin(struct buf *b) {
    acquire(&bcache.lock);
    b->refcnt++;
    release(&bcache.lock);
}

void bunpin(struct buf *b) {
    acquire(&bcache.lock);
    b->refcnt--;
    release(&bcache.lock);
}