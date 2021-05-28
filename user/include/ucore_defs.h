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

struct cpu_stat {
    uint64 uptime;
    uint64 sample_duration;
    uint64 sample_busy_duration;
};


struct mem_stat
{
    uint64 physical_total;
    uint64 physical_free;
};

struct proc_stat
{
    char name[PROC_NAME_MAX];
    int pid;
    int ppid; // Parent process
    int dsid;
    int state;
    uint64 heap_sz;
    uint64 total_size;
    uint64 cpu_time; // ms, user and kernel
};
#endif // UCORE_DEFS_H
