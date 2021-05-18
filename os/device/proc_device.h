#if !defined(PROC_DEVICE_H)
#define PROC_DEVICE_H
#include <ucore/ucore.h>

int64 proc_write(char *src, int64 len, int from_user);
int64 proc_read(char *dst, int64 len, int to_user);

#endif // PROC_DEVICE_H


