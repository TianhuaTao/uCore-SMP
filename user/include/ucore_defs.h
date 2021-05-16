#if !defined(UCORE_DEFS_H)
#define UCORE_DEFS_H

#define NDIRECT 12

// On-disk inode structure
struct dinode {
    short type; // File type
    short major;          // Major device number (T_DEVICE only)
    short minor;          // Minor device number (T_DEVICE only)
    short nlink;          // Number of links to inode in file system
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

#endif // UCORE_DEFS_H
