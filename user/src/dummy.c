#include <stdio.h>
#include <ucore.h>

#define kArrLen 123
const uint32 kSeed = 202105;

uint32 sort_arr[kArrLen];

int main()
{
    sleep(10);
    // puts("hello world!");
    for (int k = 0; k < kArrLen; ++k)
        for (int j = 0; j < kArrLen; ++j)
            for (int i = 0; i < kArrLen; ++i)
                sort_arr[i] = kSeed;
    return 0;
}