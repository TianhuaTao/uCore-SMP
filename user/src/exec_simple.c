#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    puts("execute hello");
    exec("hello");
    return 0;
}