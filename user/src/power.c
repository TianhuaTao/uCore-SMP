#include <stdio.h>
#include <unistd.h>

int main() {
    puts("power!");
    puts("power!");
    sched_yield();
    puts("power!");
    sched_yield();
    puts("power!");
    sched_yield();
    return 0;
}