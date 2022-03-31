#if !defined(TYPES_H)
#define TYPES_H

#include <stddef.h>

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long long uint64;
//typedef unsigned long long size_t;
typedef long long int64;
typedef long long ssize_t;
typedef int err_t;
typedef int bool;
typedef uint64 pte_t;
typedef uint64 pde_t;
typedef uint64 *pagetable_t;// 512 PTEs
typedef int pid_t;// 512 PTEs
//typedef unsigned long long uintptr_t;

#define NULL ((void *)0)

#define readb(addr) (*(volatile uint8 *)(addr))
#define readw(addr) (*(volatile uint16 *)(addr))
#define readd(addr) (*(volatile uint32 *)(addr))
#define readq(addr) (*(volatile uint64 *)(addr))

#define writeb(v, addr)                      \
    {                                        \
        (*(volatile uint8 *)(addr)) = (v); \
    }
#define writew(v, addr)                       \
    {                                         \
        (*(volatile uint16 *)(addr)) = (v); \
    }
#define writed(v, addr)                       \
    {                                         \
        (*(volatile uint32 *)(addr)) = (v); \
    }
#define writeq(v, addr)                       \
    {                                         \
        (*(volatile uint64 *)(addr)) = (v); \
    }

#endif // TYPES_H
