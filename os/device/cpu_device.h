#if !defined(CPU_DEVICE_H)
#define CPU_DEVICE_H
#include <ucore/ucore.h>

int64 cpu_write(char *src, int64 len, int from_user);
int64 cpu_read(char *dst, int64 len, int to_user);

#endif // CPU_DEVICE_H


