// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include <arch/riscv.h>
#include <file/file.h>
#include <fs/buf.h>
#include <fs/fs.h>
#include <proc/proc.h>
#include <ucore/defs.h>
#include <ucore/types.h>
// there should be one superblock per disk device, but we run with
// only one device
struct superblock sb;

static struct inode *
namex(char *path, int nameiparent, char *name);
// Read the super block.
static void read_superblock(int dev, struct superblock *sb) {
    struct buf *bp;
    bp = bread(dev, 1);
    memmove(sb, bp->data, sizeof(*sb));
    brelse(bp);
}

// Init fs
void fsinit() {
    printf("[ucore] Initialize File System ...\n");
    int dev = ROOTDEV;
    read_superblock(dev, &sb);
    if (sb.magic != FSMAGIC) {
        panic("invalid file system");
    }
    printf("[ucore] File System Initialized\n");
}

// Zero a block.
static void
bzero(int dev, int bno) {
    struct buf *bp;
    bp = bread(dev, bno);
    memset(bp->data, 0, BSIZE);
    bwrite(bp);
    brelse(bp);
}

// Blocks.

// Allocate a zeroed disk block.
static uint
balloc(uint dev) {
    int b, bi, m;
    struct buf *bp;

    bp = 0;
    for (b = 0; b < sb.size; b += BPB) {
        bp = bread(dev, BBLOCK(b, sb));
        for (bi = 0; bi < BPB && b + bi < sb.size; bi++) {
            m = 1 << (bi % 8);
            if ((bp->data[bi / 8] & m) == 0) { // Is block free?
                bp->data[bi / 8] |= m;         // Mark block in use.
                bwrite(bp);
                brelse(bp);
                bzero(dev, b + bi);
                return b + bi;
            }
        }
        brelse(bp);
    }
    panic("balloc: out of blocks");
    return 0;
}

// Free a disk block.
static void
bfree(int dev, uint b) {
    struct buf *bp;
    int bi, m;

    bp = bread(dev, BBLOCK(b, sb));
    bi = b % BPB;
    m = 1 << (bi % 8);
    if ((bp->data[bi / 8] & m) == 0)
        panic("freeing free block");
    bp->data[bi / 8] &= ~m;
    bwrite(bp);
    brelse(bp);
}

struct {
    struct spinlock lock;
    struct inode inode[NINODE];
} itable;

void iinit() {
    init_spin_lock_with_name(&itable.lock, "itable");
    for (int i = 0; i < NINODE; i++) {
        init_mutex(&itable.inode[i].lock);
    }
}

static struct inode *iget(uint dev, uint inum);

// Allocate an inode on device dev.
// Mark it as allocated by  giving it type `type`.
// Returns an allocated and referenced inode.
struct inode *
ialloc(uint dev, short type) {
    int inum;
    struct buf *bp;
    struct dinode *dip;

    for (inum = 1; inum < sb.ninodes; inum++) {
        bp = bread(dev, IBLOCK(inum, sb));
        dip = (struct dinode *)bp->data + inum % IPB;
        if (dip->type == 0) { // a free inode
            memset(dip, 0, sizeof(*dip));
            dip->type = type;
            bwrite(bp);
            brelse(bp);
            return iget(dev, inum);
        }
        brelse(bp);
    }
    panic("ialloc: no inodes");
    return 0;
}

// Copy a modified in-memory inode to disk.
// Must be called after every change to an ip->xxx field
// that lives on disk.
void iupdate(struct inode *ip) {
    struct buf *bp;
    struct dinode *dip;

    bp = bread(ip->dev, IBLOCK(ip->inum, sb));
    dip = (struct dinode *)bp->data + ip->inum % IPB;
    dip->type = ip->type;
    dip->major = ip->major;
    dip->minor = ip->minor;
    dip->num_link = ip->num_link;
    dip->size = ip->size;
    memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
    bwrite(bp);
    brelse(bp);
}

// Find the inode with number inum on device dev
// and return the in-memory copy. Does not read
// it from disk.
static struct inode *
iget(uint dev, uint inum) {
    debugcore("iget");
    // TODO: add lock
    struct inode *inode_ptr, *empty;
    // Is the inode already in the table?
    empty = NULL;
    for (inode_ptr = &itable.inode[0]; inode_ptr < &itable.inode[NINODE]; inode_ptr++) {
        if (inode_ptr->ref > 0 && inode_ptr->dev == dev && inode_ptr->inum == inum) {
            inode_ptr->ref++;
            return inode_ptr;
        }
        if (empty == 0 && inode_ptr->ref == 0) // Remember empty slot.
            empty = inode_ptr;
    }

    // Recycle an inode entry.
    if (empty == NULL)
        panic("iget: no inodes");

    inode_ptr = empty;
    inode_ptr->dev = dev;
    inode_ptr->inum = inum;
    inode_ptr->ref = 1;
    inode_ptr->valid = 0;
    return inode_ptr;
}

// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode *
idup(struct inode *ip) {
    acquire(&itable.lock);
    ip->ref++;
    release(&itable.lock);
    return ip;
}
// Common idiom: unlock, then put.
void iunlockput(struct inode *ip) {
    iunlock(ip);
    iput(ip);
}
// Reads the inode from disk if necessary.
void ivalid(struct inode *ip) {
    debugcore("ivalid");

    struct buf *bp;
    struct dinode *dip;
    if (ip->valid == 0) {
        bp = bread(ip->dev, IBLOCK(ip->inum, sb));
        dip = (struct dinode *)bp->data + ip->inum % IPB;
        ip->type = dip->type;
        ip->size = dip->size;
        memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
        brelse(bp);
        ip->valid = 1;
        if (ip->type == 0)
            panic("ivalid: no type");
    }
}

// Drop a reference to an in-memory inode.
// If that was the last reference, the inode table entry can
// be recycled.
// If that was the last reference and the inode has no links
// to it, free the inode (and its content) on disk.
// All calls to iput() must be inside a transaction in
// case it has to free the inode.
void iput(struct inode *ip) {
    acquire(&itable.lock);

    if (ip->ref == 1 && ip->valid && ip->num_link == 0) {
        // inode has no links and no other references: truncate and free.

        // ip->ref == 1 means no other process can have ip locked,
        // so this acquiresleep() won't block (or deadlock).
        acquire_mutex_sleep(&ip->lock);

        release(&itable.lock);

        itrunc(ip);
        ip->type = 0;
        iupdate(ip);
        ip->valid = 0;

        release_mutex_sleep(&ip->lock);

        acquire(&itable.lock);
    }

    ip->ref--;
    release(&itable.lock);
}

// Inode content
//
// The content (data) associated with each inode is stored
// in blocks on the disk. The first NDIRECT block numbers
// are listed in ip->addrs[].  The next NINDIRECT blocks are
// listed in block ip->addrs[NDIRECT].

// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
static uint
bmap(struct inode *ip, uint bn) {
    uint addr, *a;
    struct buf *bp;

    if (bn < NDIRECT) {
        if ((addr = ip->addrs[bn]) == 0)
            ip->addrs[bn] = addr = balloc(ip->dev);
        return addr;
    }
    bn -= NDIRECT;

    if (bn < NINDIRECT) {
        // Load indirect block, allocating if necessary.
        if ((addr = ip->addrs[NDIRECT]) == 0)
            ip->addrs[NDIRECT] = addr = balloc(ip->dev);
        bp = bread(ip->dev, addr);
        a = (uint *)bp->data;
        if ((addr = a[bn]) == 0) {
            a[bn] = addr = balloc(ip->dev);
            bwrite(bp);
        }
        brelse(bp);
        return addr;
    }

    panic("bmap: out of range");
    return 0;
}

// Truncate inode (discard contents).
// Caller must hold ip->lock.
void itrunc(struct inode *ip) {
    int i, j;
    struct buf *bp;
    uint *a;

    for (i = 0; i < NDIRECT; i++) {
        if (ip->addrs[i]) {
            bfree(ip->dev, ip->addrs[i]);
            ip->addrs[i] = 0;
        }
    }

    if (ip->addrs[NDIRECT]) {
        bp = bread(ip->dev, ip->addrs[NDIRECT]);
        a = (uint *)bp->data;
        for (j = 0; j < NINDIRECT; j++) {
            if (a[j])
                bfree(ip->dev, a[j]);
        }
        brelse(bp);
        bfree(ip->dev, ip->addrs[NDIRECT]);
        ip->addrs[NDIRECT] = 0;
    }

    ip->size = 0;
    iupdate(ip);
}

// Read data from inode.
// If user_dst==1, then dst is a user virtual address;
// otherwise, dst is a kernel address.
int readi(struct inode *ip, int user_dst, void *dst, uint off, uint n) {
    // debugcore("readi");
    uint tot, m;
    struct buf *bp;

    if (off > ip->size || off + n < off)
        return 0;
    if (off + n > ip->size)
        n = ip->size - off;

    for (tot = 0; tot < n; tot += m, off += m, dst += m) {
        bp = bread(ip->dev, bmap(ip, off / BSIZE));
        m = MIN(n - tot, BSIZE - off % BSIZE);
        if (either_copyout((char *)dst, (char *)bp->data + (off % BSIZE), m, user_dst) == -1) {
            brelse(bp);
            tot = -1;
            break;
        }
        brelse(bp);
    }
    return tot;
}

// Write data to inode.
// Caller must hold ip->lock.
// If user_src==1, then src is a user virtual address;
// otherwise, src is a kernel address.
// Returns the number of bytes successfully written.
// If the return value is less than the requested n,
// there was an error of some kind.
int writei(struct inode *ip, int user_src, void *src, uint off, uint n) {
    uint tot, m;
    struct buf *bp;

    if (off > ip->size || off + n < off)
        return -1;
    if (off + n > MAXFILE * BSIZE)
        return -1;

    for (tot = 0; tot < n; tot += m, off += m, src += m) {
        bp = bread(ip->dev, bmap(ip, off / BSIZE));
        m = MIN(n - tot, BSIZE - off % BSIZE);
        if (either_copyin((char *)bp->data + (off % BSIZE), (char *)src, m, user_src) == -1) {
            brelse(bp);
            break;
        }
        bwrite(bp);
        brelse(bp);
    }

    if (off > ip->size)
        ip->size = off;

    // write the i-node back to disk even if the size didn't change
    // because the loop above might have called bmap() and added a new
    // block to ip->addrs[].
    iupdate(ip);

    return tot;
}

int namecmp(const char *s, const char *t) {
    return strncmp(s, t, DIRSIZ);
}
// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode *
dirlookup(struct inode *dp, char *name, uint *poff) {
    uint off, inum;
    struct dirent de;

    if (dp->type != T_DIR)
        panic("dirlookup not DIR");

    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, 0, &de, off, sizeof(de)) != sizeof(de))
            panic("dirlookup read");
        if (de.inum == 0)
            continue;
        if (namecmp(name, de.name) == 0) {
            // entry matches path element
            if (poff)
                *poff = off;
            inum = de.inum;
            return iget(dp->dev, inum);
        }
    }

    return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
int dirlink(struct inode *dp, char *name, uint inum) {
    int off;
    struct dirent de;
    struct inode *ip;
    // Check that name is not present.
    if ((ip = dirlookup(dp, name, 0)) != 0) {
        iput(ip);
        return -1;
    }

    // Look for an empty dirent.
    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, 0,  &de, off, sizeof(de)) != sizeof(de))
            panic("dirlink read");
        if (de.inum == 0)
            break;
    }
    strncpy(de.name, name, DIRSIZ);
    de.inum = inum;
    if (writei(dp, FALSE,  &de, off, sizeof(de)) != sizeof(de))
        panic("dirlink");
    return 0;
}

struct inode *root_dir() {
    debugcore("root_dir");
    struct inode *r = iget(ROOTDEV, ROOTINO);
    ivalid(r);
    return r;
}

struct inode *
namei(char *path) {
    char name[DIRSIZ];
    return namex(path, 0, name);
}

// Unlock the given inode.
void iunlock(struct inode *ip) {
    if (ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
        panic("iunlock");

    release_mutex_sleep(&ip->lock);
}

// Lock the given inode.
// Reads the inode from disk if necessary.
void ilock(struct inode *ip) {
    struct buf *bp;
    struct dinode *dip;

    if (ip == 0 || ip->ref < 1)
        panic("ilock");

    acquire_mutex_sleep(&ip->lock);

    if (ip->valid == 0) {
        bp = bread(ip->dev, IBLOCK(ip->inum, sb));
        dip = (struct dinode *)bp->data + ip->inum % IPB;
        ip->type = dip->type;
        ip->major = dip->major;
        ip->minor = dip->minor;
        ip->num_link = dip->num_link;
        ip->size = dip->size;
        memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
        brelse(bp);
        ip->valid = 1;
        if (ip->type == 0)
            panic("ilock: no type");
    }
}

struct inode *
nameiparent(char *path, char *name) {
    return namex(path, 1, name);
}

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
static char *
skipelem(char *path, char *name) {
    char *s;
    int len;

    while (*path == '/')
        path++;
    if (*path == 0)
        return 0;
    s = path;
    while (*path != '/' && *path != 0)
        path++;
    len = path - s;
    if (len >= DIRSIZ)
        memmove(name, s, DIRSIZ);
    else {
        memmove(name, s, len);
        name[len] = 0;
    }
    while (*path == '/')
        path++;
    return path;
}

// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Must be called inside a transaction since it calls iput().
static struct inode *
namex(char *path, int nameiparent, char *name) {
    struct inode *ip, *next;

    if (*path == '/')
        ip = iget(ROOTDEV, ROOTINO);
    else
        ip = idup(curr_proc()->cwd);

    while ((path = skipelem(path, name)) != 0) {
        ilock(ip);
        if (ip->type != T_DIR) {
            iunlockput(ip);
            return 0;
        }
        if (nameiparent && *path == '\0') {
            // Stop one level early.
            iunlock(ip);
            return ip;
        }
        if ((next = dirlookup(ip, name, 0)) == 0) {
            iunlockput(ip);
            return 0;
        }
        iunlockput(ip);
        ip = next;
    }
    if (nameiparent) {
        iput(ip);
        return 0;
    }
    return ip;
}
