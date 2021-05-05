#ifndef __STDIO_H__
#define __STDIO_H__

#define stdin  0
#define stdout 1
#define stderr 2

int getchar();
int putchar(int);
int puts(const char* s);
void printf(const char* fmt, ...);
#endif // __STDIO_H__
