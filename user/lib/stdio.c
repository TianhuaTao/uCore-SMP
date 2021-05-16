#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ucore.h>

int getchar() {
    char byte = 0;
    read(stdin, &byte, 1);
    return byte;
}

int putchar(int c)
{
    static char put[2] = {0, 0};
    put[0] = c;
    return write(stdout, put, 1);
}

int puts(const char* s)
{
    int r;
    r = -(write(stdout, (void*)s, strlen(s)) < 0 || putchar('\n') < 0);
    return r;
}

static char digits[] = "0123456789abcdef";

static void printint(int xx, int base, int sign) {
    char buf[16];
    int i;
    uint8 x;

    if (sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign)
        buf[i++] = '-';

    while (--i >= 0)
        putchar(buf[i]);
}

static void printptr(uint64 x) {
    int i;
    putchar('0');
    putchar('x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
        putchar(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the console. only understands %d, %x, %p, %s.
void printf(const char* fmt, ...) {
    va_list ap;
    int i, c;
    char *s;

    va_start(ap, fmt);
    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            putchar(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0)
            break;
        switch (c) {
            case 'd':
                printint(va_arg(ap, int), 10, 1);
                break;
            case 'x':
                printint(va_arg(ap, int), 16, 1);
                break;
            case 'p':
                printptr(va_arg(ap, uint64));
                break;
            case 's':
                if ((s = va_arg(ap, char *)) == 0)
                s = "(null)";
                for (; *s; s++)
                    putchar(*s);
                break;
            case '%':
                putchar('%');
                break;
            default:
                // Print unknown % sequence to draw attention.
                putchar('%');
                putchar(c);
                break;
        }
    }
    va_end(ap);
}