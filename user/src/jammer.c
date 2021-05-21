#include <stdio.h>
#include <ucore.h>

#define PGSIZE 4096
#define PGROUNDUP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
const uint32 kArrLen = 524288;
const uint32 kSeed = 22501202;

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
    int times = 5, x = kSeed;
    char arr_name[] = "jammerx";
    arr_name[6] += getpid();
    uint32 *arr = (uint32 *)sharedmem(arr_name, PGROUNDUP(kArrLen * sizeof(uint32)));

    while (times-- > 0)
        for (int i = 0; i < kArrLen; ++i, x = NextInteger(x))
            arr[x & (kArrLen - 1)] += x;
    return time_ms() - start_time;
}