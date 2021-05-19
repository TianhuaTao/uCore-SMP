#if !defined(CONSOLE_H)
#define CONSOLE_H
#include <ucore/ucore.h>

int64 console_write(char *src, int64 len, int from_user);
int64 console_read(char *dst, int64 len, int to_user);

#endif // CONSOLE_H


