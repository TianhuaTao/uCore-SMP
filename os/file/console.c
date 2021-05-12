#include <proc/proc.h>
#include <sbi/sbi.h>
int64 console_write(char *src, int64 len, int from_user) {
    int64 i;
    for (i = 0; i < len; i++) {
        char c;
        if (either_copyin(&c, src + i, 1, from_user) == -1)
            break;
        sbi_console_putchar(c);
    }

    return i;
}

int64 console_read(char *dst, int64 len, int to_user) {
    int64 i;
    for (i = 0; i < len; i++) {
        char c;
        do {
            c = sbi_console_getchar();
        } while (c == -1);

        if (either_copyin(dst + i, &c, 1, to_user) == -1)
            break;
    }
    return i;
}
