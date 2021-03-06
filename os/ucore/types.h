#if !defined(TYPES_H)
#define TYPES_H

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long long uint64;
typedef unsigned long long size_t;
typedef long long int64;
typedef long long ssize_t;
typedef int err_t;
typedef int bool;
typedef uint64 pte_t;
typedef uint64 pde_t;
typedef uint64 *pagetable_t;// 512 PTEs
typedef int pid_t;// 512 PTEs
#define NULL ((void *)0)

#endif // TYPES_H
