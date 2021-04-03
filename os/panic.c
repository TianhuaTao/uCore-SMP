#include "defs.h"

void loop() {
    for(;;);
}
void set_pr_locking(int value);

void panic(char *s)
{
    set_pr_locking(0);
    printf("panic: ");
    printf(s);
    printf("\n");
    shutdown();
}
