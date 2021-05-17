#ifndef __STRING_H__
#define __STRING_H__

#include "stddef.h"

int isspace(int c);
int isdigit(int c);
int atoi(const char *s);
void *memset(void *dest, int c, size_t n);
int strcmp(const char *l, const char *r);
int strncmp(const char *_l, const char *_r, size_t n);
size_t strlen(const char *);
size_t strnlen(const char *s, size_t n);
char* stpncpy(char *restrict d, const char *restrict s, size_t n);
int stpncmp(const char *_l, const char *_r, size_t n);
char *
strcpy(char *s, const char *t);
void *
memmove(void *vdst, const void *vsrc, int n);
#endif // __STRING_H__
