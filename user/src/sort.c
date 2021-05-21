#include <stddef.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>

#define PGSIZE 4096
#define PGROUNDUP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define RADIX_BITS (16)
#define RADIX (1 << RADIX_BITS)
#define RADIX_BITMASK (RADIX - 1)

// 伪随机数生成器
inline uint32 NextInteger(uint32 x)
{
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

// 根据给定的种子和数量生成待排序的数据
void InitSortData(uint32 arr[], uint32 n, uint32 seed)
{
    for (uint32 i = 0; i < n; i++)
    {
        seed = NextInteger(seed);
        arr[i] = seed;
    }
    return;
}

// 检查排序结果；这里只检查了数组是否为升序
int CheckSortResult(uint32 arr[], uint32 n)
{
    int res = 1;
    --n;
    for (uint32 i = 0; i < n - 1; ++i)
        res &= (arr[i] <= arr[i + 1]);
    return res;
}

char arr_name[] = "arr0", cnt_name[] = "cnt0", tmp_name[] = "tmp0";
int pid;
uint32 *cnt, *tmp;

void RadixSort(uint32 arr[], uint32 n)
{
    uint32 digit;
    for (int shift_bits = 0; shift_bits < 32; shift_bits += RADIX_BITS)
    {
        // 对于每个数位使用计数排序。基数设为2的整数幂，这样可以用位运算代替乘除和取模
        memset(cnt, 0, RADIX * sizeof(uint32));
        for (int j = 0; j < n; ++j)
        {
            digit = (arr[j] >> shift_bits) & RADIX_BITMASK;
            ++cnt[digit];
        }
        for (uint64 j = 1; j < RADIX; ++j)
            cnt[j] += cnt[j - 1];
        for (int j = n - 1; j >= 0; --j)
        {
            digit = (arr[j] >> shift_bits) & RADIX_BITMASK;
            tmp[--cnt[digit]] = arr[j];
        }
        memmove(arr, tmp, n * sizeof(uint32));
    }
    return;
}

int main(void)
{
    uint64 start_time = time_ms();
    // 随机数种子和待排序的数据数量，可自行修改
    uint32 seed = 2021;
    uint32 n = 501202;
    pid = getpid();
    arr_name[3] += pid, cnt_name[3] += pid, cnt_name[3] += pid;
    uint32 *arr = (uint32 *)sharedmem(arr_name, PGROUNDUP(n * sizeof(uint32)));
    tmp = (uint32 *)sharedmem(tmp_name, PGROUNDUP(n * sizeof(uint32)));
    cnt = (uint32 *)sharedmem(cnt_name, PGROUNDUP(RADIX * sizeof(uint32)));
    if (arr == NULL || tmp == NULL || cnt == NULL)
        return -233;

    InitSortData(arr, n, seed);
    RadixSort(arr, n);
    assert(CheckSortResult(arr, n) == 1, -2);

    return time_ms() - start_time;
}