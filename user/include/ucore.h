#if !defined(__UCORE_H__)
#define __UCORE_H__

#define NDIRECT 12

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned long long size_t;
typedef long long int64;
typedef long long ssize_t;
typedef int err_t;
typedef int bool;
typedef uint64 pte_t;
typedef uint64 pde_t;
typedef uint64 *pagetable_t; // 512 PTEs
#define NULL ((void *)0)

// File type
#define T_DIR 1    // Directory
#define T_FILE 2   // File
#define T_DEVICE 3 // Device

// On-disk inode structure
struct dinode {
    short type; // File type
    short pad[3];
    uint size;               // Size of file (bytes)
    uint addrs[NDIRECT + 1]; // Data block addresses
};

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14
struct stat {
    int dev;     // File system's disk device
    uint ino;    // Inode number
    short type;  // Type of file
    short nlink; // Number of links to file
    size_t size; // Size of file in bytes
};

struct dirent {
    ushort inum;
    char name[DIRSIZ];
};

int fstat(int fd, struct stat *);

int stat(const char *n, struct stat *st);
#endif // __UCORE_H__
