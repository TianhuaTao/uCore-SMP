#include <ucore/defs.h>

void loop() {
    for(;;);
}
void set_printf_use_lock(int value);

void panic(char *s)
{
    // set_printf_use_lock(FALSE);
    printf("panic: ");
    printf(s);
    printf("\n");
    shutdown();
}
