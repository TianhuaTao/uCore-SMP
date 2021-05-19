#include <stdio.h>
#include <ucore.h>

#define MATSIZE 100
const uint32 SEED = 20210522;

int64 mata[MATSIZE][MATSIZE], matb[MATSIZE][MATSIZE], matc[MATSIZE][MATSIZE];

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

    int i, j, k, x = SEED, size = MATSIZE;
    for (i = 0; i < size; i++)
        for (j = 0; j < size; j++)
        {
            x = NextInteger(x);
            mata[i][j] = x;
            x = NextInteger(x);
            matb[i][j] = x;
        }

    int times = 30;

    while (times-- > 0)
    {
        for (i = 0; i < size; i++)
            for (j = 0; j < size; j++)
                for (matc[i][j] = 0, k = 0; k < size; k++)
                    matc[i][j] += mata[i][k] * matb[k][j];

        for (i = 0; i < size; i++)
            for (j = 0; j < size; j++)
            {
                x = NextInteger(x);
                mata[i][j] = x;
                matb[i][j] = matc[i][j];
            }
    }

    return time_ms() - start_time;
}
