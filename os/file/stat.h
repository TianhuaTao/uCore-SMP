#if !defined(STAT_H)
#define STAT_H
#include <ucore/types.h>
struct stat {
    int dev;     // File system's disk device
    uint ino;    // Inode number
    short type;  // Type of file
    short nlink; // Number of links to file
    size_t size; // Size of file in bytes
};

#endif // STAT_H
