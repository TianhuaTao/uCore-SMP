#include <ucore/defs.h>
#include <lock/lock.h>
#include <stdarg.h>
static char digits[] = "0123456789abcdef";

// lock to avoid interleaving concurrent printf's.
static struct {
  struct spinlock lock;
  int use_lock;
} printf_setting;

/**
 * Ture on/off printf's locking
 */
void set_printf_use_lock(int value){
    printf_setting.use_lock = value;
}

/**
 * init the lock for printf
 */
void printfinit(void)
{
    init_spin_lock_with_name(&printf_setting.lock, "printf_setting.lock");
    printf_setting.use_lock = 1;
}

static void
printint(int xx, int base, int sign) {
    char buf[16];
    int i;
    uint x;

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
        sbi_console_putchar(buf[i]);
}

static void
printptr(uint64 x) {
    int i;
    sbi_console_putchar('0');
    sbi_console_putchar('x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
        sbi_console_putchar(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the console. only understands %d, %x, %p, %s.
void printf(char *fmt, ...) {
    va_list ap;
    int i, c;
    char *s;

    if (fmt == 0)
        panic("null fmt");

    int use_lock = printf_setting.use_lock;
    if(use_lock){
        acquire(&printf_setting.lock);
    }
    va_start(ap, fmt);
    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            sbi_console_putchar(c);
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
                    sbi_console_putchar(*s);
                break;
            case '%':
                sbi_console_putchar('%');
                break;
            default:
                // Print unknown % sequence to draw attention.
                sbi_console_putchar('%');
                sbi_console_putchar(c);
                break;
        }
    }
    if(use_lock)
        release(&printf_setting.lock);
}

// Print to the console. only understands %d, %x, %p, %s.
// Does NOT use lock
void printf_k(char *fmt, ...) {
    va_list ap;
    int i, c;
    char *s;

    if (fmt == 0)
        panic("null fmt");

    va_start(ap, fmt);
    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            sbi_console_putchar(c);
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
                    sbi_console_putchar(*s);
                break;
            case '%':
                sbi_console_putchar('%');
                break;
            default:
                // Print unknown % sequence to draw attention.
                sbi_console_putchar('%');
                sbi_console_putchar(c);
                break;
        }
    }

}