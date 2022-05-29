#if !defined(INODE_H)
#define INODE_H
#include <ucore/ucore.h>
#include <lock/lock.h>
#include <fatfs/ff.h>
#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

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
    FIL* FAT_FILE;
    DIR* DIRECTORY;
};

void inode_table_init();

struct inode *iget(uint dev);
void iput(struct inode *ip);

struct inode * idup(struct inode *ip);

int readi(struct inode *ip, int user_dst, void *dst, uint off, uint n);
int writei(struct inode *ip, int user_src, void *src, uint off, uint n);

void ilock(struct inode *ip);
void iunlock(struct inode *ip);

struct inode *inode_by_name(char *path);
struct inode *inode_parent_by_name(char *path, char *name);

void itrunc(struct inode *ip);
#endif // INODE_H
