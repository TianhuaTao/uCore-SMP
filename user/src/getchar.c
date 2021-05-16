#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>

int main() {
    printf("getchar 10:");
    for(int i = 0; i < 10; ++i) {
        int c = getchar();
        putchar(c);
    }
    putchar('\n');
    return 0;
}