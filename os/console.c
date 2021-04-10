#include "defs.h"

void consputc(int c) {
    console_putchar(c);
}

char consgetc() {
    return console_getchar();
}
