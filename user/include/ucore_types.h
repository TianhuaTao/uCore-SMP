#if !defined(UCORE_TYPES_H)
#define UCORE_TYPES_H

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
typedef int pid_t;
#define NULL ((void *)0)

// File type
#define T_DIR 1    // Directory
#define T_FILE 2   // File
#define T_DEVICE 3 // Device

#define NPROC (256)
#define KSTACK_SIZE (4096)
#define USTACK_SIZE (4096)
#define TRAPFRAME_SIZE (4096)
#define FD_MAX (16)
#define PROC_NAME_MAX (16)

enum procstate {
    UNUSED = 0,
    USED,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE
};
#endif // UCORE_TYPES_H
