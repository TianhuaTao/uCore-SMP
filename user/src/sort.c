#include <stddef.h>
#include <stdlib.h>
#include <ucore.h>

#define kArrLen 12345
const uint32 kSeed = 202105;

uint32 sort_arr[kArrLen];

inline uint32 NextInteger(uint32 x)
{
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

void InitSortData(uint32 arr[], int n, uint32 seed)
{
    for (int i = 0; i < n; ++i)
    {
        seed = NextInteger(seed);
        arr[i] = seed;
    }
    return;
}

void InsertionSort(uint32 arr[], uint32 n)
{
    for (int i = 1; i < n; ++i)
    {
        int j = i - 1;
        uint32 val = arr[i];
        for (; j >= 0 && arr[j] > val; --j)
            arr[j + 1] = arr[j];
        arr[j + 1] = val;
    }
}

int CheckSortResult(uint32 arr[], uint32 n)
{
    int res = 1;
    --n;
    for (size_t i = 0; i < n; ++i)
        res &= (arr[i] <= arr[i + 1]);
    return res;
}

int main(void)
{
    uint64 start_time = time_ms();
    InitSortData(sort_arr, kArrLen, kSeed);
    InsertionSort(sort_arr, kArrLen);
    assert(CheckSortResult(sort_arr, kArrLen) == 1, -10);
    return time_ms() - start_time;
}
