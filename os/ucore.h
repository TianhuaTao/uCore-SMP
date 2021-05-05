#if !defined(UCORE_H)
#define UCORE_H

#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) ); })

#include "types.h"
#include "cpu.h"
#include "defs.h"
#include "log.h"
#include "riscv.h"

#endif // UCORE_H
