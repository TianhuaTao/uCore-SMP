#if !defined(MEM_DEVICE_H)
#define MEM_DEVICE_H
#include <ucore/ucore.h>

int64 mem_write(char *src, int64 len, int from_user);
int64 mem_read(char *dst, int64 len, int to_user);

#endif // MEM_DEVICE_H


