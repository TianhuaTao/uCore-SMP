#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>

int main(int argc, char *argv[])
{
    long long p;

    p = setpriority(100);
    assert(p == 100, -1);

    p = setpriority(20);
    assert(p == 20, -2);

    p = setpriority(-5);
    assert(p == -1, -3);

    p = getpriority();
    assert(p == 20, -4);

    return 0;
}