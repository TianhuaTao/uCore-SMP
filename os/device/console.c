#include <proc/proc.h>
#include <sbi/sbi.h>
#include <utils/log.h>
#include "console.h"
void console_init(){
    device_rw_handler[CONSOLE].read = console_read;
    device_rw_handler[CONSOLE].write = console_write;
}


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
        int c;
        do {
            c = sbi_console_getchar();
        } while (c == -1);

        if (either_copyout(dst + i, &c, 1, to_user) == -1){
            infof("console_read either_copyout failed");
            break;
        }
    }
    return i;
}
