#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>

const int kMax = 52021;

inline int IsPrime(int x)
{
    for (int t = 2; t <= x / 2; ++t)
        if (x % t == 0)
            return 0;
    return 1;
}

int a[2];
int main()
{
    uint64 start_time = time_ms();
    int prime_num = 0;
        for (int i = 2; i < kMax; ++i)
            prime_num += IsPrime(i);
    assert(prime_num > 0, -2);
    a[1] = prime_num;
    return time_ms() - start_time;
}