#include <stdio.h>
#include <ucore.h>

#define kArrLen 8192
const uint32 kSeed = 22501202;

uint32 arr[kArrLen];

inline uint32 NextInteger(uint32 x)
{
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

int main()
{
    uint64 start_time = time_ms();
    int times = 1000, x = kSeed;
    while (times-- > 0)
        for (int i = 0; i < kArrLen; ++i, x = NextInteger(x))
            arr[x & (kArrLen - 1)] += x;
    return time_ms() - start_time;
}