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
inode_or_parent_by_name(char *path, int nameiparent, char *name);
// Read the super block.
static void read_superblock(int dev, struct superblock *sb)
{
    struct buf *bp;
    bp = acquire_buf_and_read(dev, 1);
    memmove(sb, bp->data, sizeof(*sb));
    release_buf(bp);
}

// Init fs
void fsinit()
{
    printf("[ucore] Initialize File System ...\n");
    int dev = ROOTDEV;
    read_superblock(dev, &sb);
    if (sb.magic != FSMAGIC)
    {
        panic("invalid file system");
    }
    printf("[ucore] File System Initialized\n");
}

// Zero a block.
static void
set_block_to_zero(int dev, int bno)
{
    struct buf *bp;
    bp = acquire_buf_and_read(dev, bno);
    memset(bp->data, 0, BSIZE);
    write_buf_to_disk(bp);
    release_buf(bp);
}

// Blocks.

// Allocate a zeroed disk block.
static uint
alloc_zeroed_block(uint dev)
{
    int base_block_id;
    int local_block_id;
    int bit_mask;
    struct buf *bitmap_block;

    bitmap_block = NULL;
    for (base_block_id = 0; base_block_id < sb.total_blocks; base_block_id += BITS_PER_BITMAP_BLOCK)
    {
        // the corresponding bitmap block
        bitmap_block = acquire_buf_and_read(dev, BITMAP_BLOCK_CONTAINING(base_block_id, sb));

        // iterate all bits in this bitmap block
        for (local_block_id = 0; local_block_id < BITS_PER_BITMAP_BLOCK && base_block_id + local_block_id < sb.total_blocks; local_block_id++)
        {
            bit_mask = 1 << (local_block_id % 8);
            if ((bitmap_block->data[local_block_id / 8] & bit_mask) == 0)
            {                          
                // the block free
                bitmap_block->data[local_block_id / 8] |= bit_mask; // Mark block in use.
                write_buf_to_disk(bitmap_block);
                release_buf(bitmap_block);
                set_block_to_zero(dev, base_block_id + local_block_id);
                return base_block_id + local_block_id;
            }
        }
        release_buf(bitmap_block);
    }
    panic("alloc_zeroed_block: out of blocks");
    return 0;
}

// Free a disk block.
static void
free_disk_block(int dev, uint block_id)
{
    struct buf *bitmap_block;
    int bit_offset; // in bitmap block
    int mask ;

    bitmap_block = acquire_buf_and_read(dev, BITMAP_BLOCK_CONTAINING(block_id, sb));
    bit_offset = block_id % BITS_PER_BITMAP_BLOCK;
    mask = 1 << (bit_offset % 8);
    if ((bitmap_block->data[bit_offset / 8] & mask) == 0)
        panic("freeing free block");
    bitmap_block->data[bit_offset / 8] &= ~mask;
    write_buf_to_disk(bitmap_block);
    release_buf(bitmap_block);
}

struct
{
    struct spinlock lock;
    struct inode inode[NINODE];
} itable;

void iinit()
{
    init_spin_lock_with_name(&itable.lock, "itable");
    for (int i = 0; i < NINODE; i++)
    {
        init_mutex(&itable.inode[i].lock);
    }
}

static struct inode *iget(uint dev, uint inum);

// Allocate an inode on device dev.
// Mark it as allocated by  giving it type `type`.
// Returns an allocated and referenced inode.
struct inode *
alloc_disk_inode(uint dev, short type)
{
    int inum;
    struct buf *bp;
    struct dinode *disk_inode;

    for (inum = 1; inum < sb.ninodes; inum++)
    {
        bp = acquire_buf_and_read(dev, BLOCK_CONTAINING_INODE(inum, sb));
        disk_inode = (struct dinode *)bp->data + inum % INODES_PER_BLOCK;
        if (disk_inode->type == 0)
        { 
            // a free inode
            memset(disk_inode, 0, sizeof(*disk_inode));
            disk_inode->type = type;
            write_buf_to_disk(bp);
            release_buf(bp);
            return iget(dev, inum);
        }
        release_buf(bp);
    }
    panic("ialloc: no inodes");
    return 0;
}

// Copy a modified in-memory inode to disk.
// Must be called after every change to an ip->xxx field
// that lives on disk.
void iupdate(struct inode *ip)
{
    struct buf *bp;
    struct dinode *dip;

    bp = acquire_buf_and_read(ip->dev, BLOCK_CONTAINING_INODE(ip->inum, sb));
    dip = (struct dinode *)bp->data + ip->inum % INODES_PER_BLOCK;
    dip->type = ip->type;
    dip->major = ip->major;
    dip->minor = ip->minor;
    dip->num_link = ip->num_link;
    dip->size = ip->size;
    memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
    write_buf_to_disk(bp);
    release_buf(bp);
}

// Find the inode with number inum on device dev
// and return the in-memory copy. Does not read
// it from disk.
static struct inode *
iget(uint dev, uint inum)
{
    debugcore("iget");

    struct inode *inode_ptr, *empty;
    acquire(&itable.lock);
    // Is the inode already in the table?
    empty = NULL;
    for (inode_ptr = &itable.inode[0]; inode_ptr < &itable.inode[NINODE]; inode_ptr++)
    {
        if (inode_ptr->ref > 0 && inode_ptr->dev == dev && inode_ptr->inum == inum)
        {
            inode_ptr->ref++;
            release(&itable.lock);
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
    release(&itable.lock);
    return inode_ptr;
}

// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode *
idup(struct inode *ip)
{
    KERNEL_ASSERT(ip != NULL, "inode can not be NULL");
    acquire(&itable.lock);
    ip->ref++;
    release(&itable.lock);
    return ip;
}
// Common idiom: unlock, then put.
void iunlockput(struct inode *ip)
{
    iunlock(ip);
    iput(ip);
}
// Reads the inode from disk if necessary.
void ivalid(struct inode *ip)
{
    debugcore("ivalid");

    struct buf *bp;
    struct dinode *dip;
    if (ip->valid == 0)
    {
        bp = acquire_buf_and_read(ip->dev, BLOCK_CONTAINING_INODE(ip->inum, sb));
        dip = (struct dinode *)bp->data + ip->inum % INODES_PER_BLOCK;
        ip->type = dip->type;
        ip->size = dip->size;
        memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
        release_buf(bp);
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
void iput(struct inode *ip)
{
    tracecore("iput");
    acquire(&itable.lock);

    if (ip->ref == 1 && ip->valid && ip->num_link == 0)
    {
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
bmap(struct inode *ip, uint bn)
{
    uint addr, *a;
    struct buf *bp;

    if (bn < NDIRECT)
    {
        if ((addr = ip->addrs[bn]) == 0)
            ip->addrs[bn] = addr = alloc_zeroed_block(ip->dev);
        return addr;
    }
    bn -= NDIRECT;

    if (bn < NINDIRECT)
    {
        // Load indirect block, allocating if necessary.
        if ((addr = ip->addrs[NDIRECT]) == 0)
            ip->addrs[NDIRECT] = addr = alloc_zeroed_block(ip->dev);
        bp = acquire_buf_and_read(ip->dev, addr);
        a = (uint *)bp->data;
        if ((addr = a[bn]) == 0)
        {
            a[bn] = addr = alloc_zeroed_block(ip->dev);
            write_buf_to_disk(bp);
        }
        release_buf(bp);
        return addr;
    }

    panic("bmap: out of range");
    return 0;
}

// Truncate inode (discard contents).
// Caller must hold ip->lock.
void itrunc(struct inode *ip)
{
    tracecore("itrunc");
    int i, j;
    struct buf *bp;
    uint *a;

    for (i = 0; i < NDIRECT; i++)
    {
        if (ip->addrs[i])
        {
            free_disk_block(ip->dev, ip->addrs[i]);
            ip->addrs[i] = 0;
        }
    }

    if (ip->addrs[NDIRECT])
    {
        bp = acquire_buf_and_read(ip->dev, ip->addrs[NDIRECT]);
        a = (uint *)bp->data;
        for (j = 0; j < NINDIRECT; j++)
        {
            if (a[j])
                free_disk_block(ip->dev, a[j]);
        }
        release_buf(bp);
        free_disk_block(ip->dev, ip->addrs[NDIRECT]);
        ip->addrs[NDIRECT] = 0;
    }

    ip->size = 0;
    iupdate(ip);
}

// Read data from inode.
// If user_dst==1, then dst is a user virtual address;
// otherwise, dst is a kernel address.
int readi(struct inode *ip, int user_dst, void *dst, uint off, uint n)
{
    // debugcore("readi");
    uint tot, m;
    struct buf *bp;

    if (off > ip->size || off + n < off)
        return 0;
    if (off + n > ip->size)
        n = ip->size - off;

    for (tot = 0; tot < n; tot += m, off += m, dst += m)
    {
        bp = acquire_buf_and_read(ip->dev, bmap(ip, off / BSIZE));
        m = MIN(n - tot, BSIZE - off % BSIZE);
        if (either_copyout((char *)dst, (char *)bp->data + (off % BSIZE), m, user_dst) == -1)
        {
            release_buf(bp);
            tot = -1;
            break;
        }
        release_buf(bp);
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
int writei(struct inode *ip, int user_src, void *src, uint off, uint n)
{
    uint tot, m;
    struct buf *bp;

    if (off > ip->size || off + n < off)
        return -1;
    if (off + n > MAXFILE * BSIZE)
        return -1;

    for (tot = 0; tot < n; tot += m, off += m, src += m)
    {
        bp = acquire_buf_and_read(ip->dev, bmap(ip, off / BSIZE));
        m = MIN(n - tot, BSIZE - off % BSIZE);
        if (either_copyin((char *)bp->data + (off % BSIZE), (char *)src, m, user_src) == -1)
        {
            release_buf(bp);
            break;
        }
        write_buf_to_disk(bp);
        release_buf(bp);
    }

    if (off > ip->size)
        ip->size = off;

    // write the i-node back to disk even if the size didn't change
    // because the loop above might have called bmap() and added a new
    // block to ip->addrs[].
    iupdate(ip);

    return tot;
}

int namecmp(const char *s, const char *t)
{
    return strncmp(s, t, DIRSIZ);
}
// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode *
dirlookup(struct inode *dp, char *name, uint *poff)
{
    uint off, inum;
    struct dirent de;

    if (dp->type != T_DIR)
        panic("dirlookup not DIR");

    for (off = 0; off < dp->size; off += sizeof(de))
    {
        if (readi(dp, 0, &de, off, sizeof(de)) != sizeof(de))
            panic("dirlookup read");
        if (de.inum == 0)
            continue;
        if (namecmp(name, de.name) == 0)
        {
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
int dirlink(struct inode *dp, char *name, uint inum)
{
    int off;
    struct dirent de;
    struct inode *ip;
    // Check that name is not present.
    if ((ip = dirlookup(dp, name, 0)) != 0)
    {
        iput(ip);
        return -1;
    }

    // Look for an empty dirent.
    for (off = 0; off < dp->size; off += sizeof(de))
    {
        if (readi(dp, FALSE, &de, off, sizeof(de)) != sizeof(de))
            panic("dirlink read");
        if (de.inum == 0)
            break;
    }
    strncpy(de.name, name, DIRSIZ);
    de.inum = inum;
    if (writei(dp, FALSE, &de, off, sizeof(de)) != sizeof(de))
        panic("dirlink");
    return 0;
}

struct inode *root_dir()
{
    debugcore("root_dir");
    struct inode *r = iget(ROOTDEV, ROOTINO);
    ivalid(r);
    return r;
}

struct inode *
inode_by_name(char *path)
{
    char name[DIRSIZ];
    return inode_or_parent_by_name(path, 0, name);
}

// Unlock the given inode.
void iunlock(struct inode *ip)
{
    if (ip == NULL || !holdingsleep(&ip->lock) || ip->ref < 1)
        panic("iunlock");

    release_mutex_sleep(&ip->lock);
}

// Lock the given inode.
// Reads the inode from disk if necessary.
void ilock(struct inode *ip)
{
    struct buf *bp;
    struct dinode *dip;

    if (ip == 0 || ip->ref < 1)
        panic("ilock");

    acquire_mutex_sleep(&ip->lock);

    if (ip->valid == 0)
    {
        bp = acquire_buf_and_read(ip->dev, BLOCK_CONTAINING_INODE(ip->inum, sb));
        dip = (struct dinode *)bp->data + ip->inum % INODES_PER_BLOCK;
        ip->type = dip->type;
        ip->major = dip->major;
        ip->minor = dip->minor;
        ip->num_link = dip->num_link;
        ip->size = dip->size;
        memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
        release_buf(bp);
        ip->valid = 1;
        if (ip->type == 0){
            errorf("dev=%d, inum=%d", (int)ip->dev, (int)ip->inum);
            panic("ilock: no type, this disk inode is invalid");
        }
    }
}

struct inode *
inode_parent_by_name(char *path, char *name)
{
    return inode_or_parent_by_name(path, 1, name);
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
skipelem(char *path, char *name)
{
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
    else
    {
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
inode_or_parent_by_name(char *path, int nameiparent, char *name)
{
    struct inode *ip, *next;
    debugcore("inode_or_parent_by_name");
    if (*path == '/')
    {
        // absolute path
        ip = iget(ROOTDEV, ROOTINO);
    }
    else
    {
        // relative path
        ip = idup(curr_proc()->cwd);
    }

    while ((path = skipelem(path, name)) != 0)
    {
        ilock(ip);
        if (ip->type != T_DIR)
        {
            iunlockput(ip);
            return 0;
        }
        if (nameiparent && *path == '\0')
        {
            // Stop one level early.
            iunlock(ip);
            return ip;
        }
        if ((next = dirlookup(ip, name, 0)) == 0)
        {
            iunlockput(ip);
            return 0;
        }
        iunlockput(ip);
        ip = next;
    }
    if (nameiparent)
    {
        iput(ip);
        return 0;
    }
    return ip;
}
// Copy stat information from inode.
// Caller must hold ip->lock.
void stati(struct inode *ip, struct stat *st)
{
    st->dev = ip->dev;
    st->ino = ip->inum;
    st->type = ip->type;
    st->nlink = ip->num_link;
    st->size = ip->size;
}
// Is the directory dp empty except for "." and ".." ?
int isdirempty(struct inode *dp)
{
    int off;
    struct dirent de;

    for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de))
    {
        if (readi(dp, 0, (void *)&de, off, sizeof(de)) != sizeof(de))
            panic("isdirempty: readi");
        if (de.inum != 0)
            return 0;
    }
    return 1;
}