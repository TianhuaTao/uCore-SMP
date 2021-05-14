#ifndef __FS_H__
#define __FS_H__

#include <ucore/types.h>
#include <file/stat.h>
// On-disk file system format.
// Both the kernel and user programs use this header file.

#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       1000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name

#define ROOTINO  1   // root i-number
#define BSIZE 1024   // block size

// Disk layout:
// [ boot block | super block | inode blocks | free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
    uint magic;     // Must be FSMAGIC
    uint size;      // Size of file system image (blocks)
    uint nblocks;   // Number of data blocks
    uint ninodes;   // Number of inodes.
    uint inodestart;// Block number of first inode block
    uint bmapstart; // Block number of first free map block
};

#define FSMAGIC 0x10203040

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// File type
#define T_DIR     1   // Directory
#define T_FILE    2   // File
#define T_DEVICE  3    // Device

// On-disk inode structure
struct dinode {
    short type;             // File type
    short major;            // Major device number (T_DEVICE only)
    short minor;            // Minor device number (T_DEVICE only)
    short num_link;            // Number of links to inode in file system
    uint size;              // Size of file (bytes)
    uint addrs[NDIRECT + 1];// Data block addresses
};

// Inodes per block.
#define IPB (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb) ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB (BSIZE * 8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b) / BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
    ushort inum;
    char name[DIRSIZ];
};
struct inode *root_dir();
struct inode *namei(char *path);
void ilock(struct inode *ip);
void iunlock(struct inode *ip);
struct inode *nameiparent(char *path, char *name);
int writei(struct inode *ip, int user_src, void *src, uint off, uint n);
int readi(struct inode *ip, int user_dst, void *dst, uint off, uint n);
void stati(struct inode *ip, struct stat *st);
int isdirempty(struct inode *dp);
int namecmp(const char *s, const char *t);
#endif //!__FS_H__
