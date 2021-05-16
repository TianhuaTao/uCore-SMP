
#define USE_RAMDISK

#include <ucore/ucore.h>
#include <fs/fs.h>
#include <fs/buf.h>
#include <utils/log.h>
#include <lock/spinlock.h>

#define NINODES 200

// Disk layout:
// [ boot block | sb block | inode blocks | free bit map | data blocks ]

int nbitmap = FSSIZE / (BSIZE * 8) + 1;
int ninodeblocks = NINODES / INODES_PER_BLOCK + 1;
int nmeta;   // Number of meta blocks (boot, sb, nlog, inode, bitmap)
int nblocks; // Number of data blocks

int fsfd;
struct superblock sb1;
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;

void balloc(int);
void wsect(uint, void *);
void winode(uint, struct dinode *);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type);
void iappend(uint inum, void *p, int n);
static void *map_block_to_ram(uint blockno);

// convert to intel byte order
ushort
xshort(ushort x)
{
    ushort y;
    uchar *a = (uchar *)&y;
    a[0] = x;
    a[1] = x >> 8;
    return y;
}

uint xint(uint x)
{
    uint y;
    uchar *a = (uchar *)&y;
    a[0] = x;
    a[1] = x >> 8;
    a[2] = x >> 16;
    a[3] = x >> 24;
    return y;
}

void wsect(uint sec, void *buf)
{
    void *mem_addr = map_block_to_ram(sec);
    memmove(mem_addr, buf, BSIZE);
    // if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
    //     panic("");
    // }
    // if (write(fsfd, buf, BSIZE) != BSIZE) {
    //     panic("");
    // }
}

void winode(uint inum, struct dinode *ip)
{
    char buf[BSIZE];
    uint bn;
    struct dinode *dip;

    bn = BLOCK_CONTAINING_INODE(inum, sb1);
    rsect(bn, buf);
    dip = ((struct dinode *)buf) + (inum % INODES_PER_BLOCK);
    *dip = *ip;
    wsect(bn, buf);
}

void rinode(uint inum, struct dinode *ip)
{
    char buf[BSIZE];
    uint bn;
    struct dinode *dip;

    bn = BLOCK_CONTAINING_INODE(inum, sb1);
    rsect(bn, buf);
    dip = ((struct dinode *)buf) + (inum % INODES_PER_BLOCK);
    *ip = *dip;
}

void rsect(uint sec, void *buf)
{
    void *mem_addr = map_block_to_ram(sec);
    memmove(buf, mem_addr, BSIZE);
    // if (lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE) {
    //     perror("lseek");
    //     exit(1);
    // }
    // if (read(fsfd, buf, BSIZE) != BSIZE) {
    //     perror("read");
    //     exit(1);
    // }
}

uint ialloc(ushort type)
{
    uint inum = freeinode++;
    struct dinode din;

    memset(&din, 0, sizeof(din));
    din.type = xshort(type);
    din.num_link = xshort(1);
    din.size = xint(0);
    winode(inum, &din);
    return inum;
}

void balloc(int used)
{
    uchar buf[BSIZE];
    int i;
    printf("balloc: first %d blocks have been allocated\n", used);
    KERNEL_ASSERT(used < BSIZE * 8, "");
    memset(buf, 0, BSIZE);
    for (i = 0; i < used; i++)
    {
        buf[i / 8] = buf[i / 8] | (0x1 << (i % 8));
    }
    printf("balloc: write bitmap block at sector %d\n", sb1.bmapstart);
    wsect(sb1.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void iappend(uint inum, void *xp, int n)
{
    char *p = (char *)xp;
    uint fbn, off, n1;
    struct dinode din;
    char buf[BSIZE];
    uint indirect[NINDIRECT];
    uint x;

    rinode(inum, &din);
    off = xint(din.size);
    while (n > 0)
    {
        fbn = off / BSIZE;
        KERNEL_ASSERT(fbn < MAXFILE, "");
        if (fbn < NDIRECT)
        {
            if (xint(din.addrs[fbn]) == 0)
            {
                din.addrs[fbn] = xint(freeblock++);
            }
            x = xint(din.addrs[fbn]);
        }
        else
        {
            if (xint(din.addrs[NDIRECT]) == 0)
            {
                din.addrs[NDIRECT] = xint(freeblock++);
            }
            rsect(xint(din.addrs[NDIRECT]), (char *)indirect);
            if (indirect[fbn - NDIRECT] == 0)
            {
                indirect[fbn - NDIRECT] = xint(freeblock++);
                wsect(xint(din.addrs[NDIRECT]), (char *)indirect);
            }
            x = xint(indirect[fbn - NDIRECT]);
        }
        n1 = min(n, (fbn + 1) * BSIZE - off);
        rsect(x, buf);
        memmove(p, buf + off - (fbn * BSIZE), n1);
        wsect(x, buf);
        n -= n1;
        off += n1;
        p += n1;
    }
    din.size = xint(off);
    winode(inum, &din);
}

#define RAM_PAGE_CNT (FSSIZE * BSIZE / PGSIZE)

void *ram_page_pool[RAM_PAGE_CNT];
struct spinlock ramdisk_lock;

static void *map_block_to_ram(uint blockno)
{
    int ram_page_id = (blockno * BSIZE) / PGSIZE;
    int ram_page_offset = (blockno * BSIZE) % PGSIZE;
    void *ram_addr = ram_page_pool[ram_page_id] + ram_page_offset;
    // phex(ram_page_id);
    // phex(ram_page_offset);
    // phex(ram_addr);
    return ram_addr;
}

void make_ram_fs()
{

    int i;
    uint rootino, off;
    struct dirent de;
    char buf[BSIZE];
    struct dinode din;

    KERNEL_ASSERT((BSIZE % sizeof(struct dinode)) == 0, "");

    // 1 fs block = 1 disk sector
    nmeta = 2 + ninodeblocks + nbitmap;
    nblocks = FSSIZE - nmeta;

    sb1.magic = FSMAGIC;
    sb1.total_blocks = xint(FSSIZE);
    sb1.num_data_blocks = xint(nblocks);
    sb1.ninodes = xint(NINODES);
    sb1.inodestart = xint(2);
    sb1.bmapstart = xint(2 + ninodeblocks);

    printf("nmeta %d (boot, super, inode blocks %u, bitmap blocks %u) blocks %d total %d\n",
           nmeta, ninodeblocks, nbitmap, nblocks, FSSIZE);

    freeblock = nmeta; // the first free block that we can allocate

    for (i = 0; i < FSSIZE; i++)
        wsect(i, zeroes);

    memset(buf, 0, sizeof(buf));
    memmove(buf, &sb1, sizeof(sb1));
    wsect(1, buf);

    rootino = ialloc(T_DIR);
    KERNEL_ASSERT(rootino == ROOTINO, "");

    memset(&de, 0, sizeof(de));
    de.inum = xshort(rootino);
    // strcpy(de.name, ".");
    de.name[0] = '.';
    de.name[1] = '\0';
    iappend(rootino, &de, sizeof(de));

    memset(&de, 0, sizeof(de));
    de.inum = xshort(rootino);
    // strcpy(de.name, "..");
    de.name[0] = '.';
    de.name[1] = '.';
    de.name[2] = '\0';

    iappend(rootino, &de, sizeof(de));

    // fix size of root inode dir
    rinode(rootino, &din);
    off = xint(din.size);
    off = ((off / BSIZE) + 1) * BSIZE;
    din.size = xint(off);
    winode(rootino, &din);

    balloc(freeblock);
}

void init_ram_disk()
{
    for (int i = 0; i < RAM_PAGE_CNT; i++)
    {
        void *page = alloc_physical_page();
        if (page == 0)
        {
            panic("init ram disk");
        }
        ram_page_pool[i] = page;
    }
    init_spin_lock_with_name(&ramdisk_lock, "ramdisk lock");

    make_ram_fs();
}

void ram_disk_rw(struct buf *b, int write)
{

    tracecore("ram_disk_rw dev=%d, blockno=%d, write=%d", b->dev, b->blockno, write);
    KERNEL_ASSERT(b->dev == ROOTDEV, "wrong device number");
    acquire(&ramdisk_lock);
    uint blockno = b->blockno;

    void *mem_addr = map_block_to_ram(blockno);
    if (write)
    {
        memmove(mem_addr, b->data, BSIZE);
    }
    else
    {
        memmove(b->data, mem_addr, BSIZE);
    }
    release(&ramdisk_lock);
}