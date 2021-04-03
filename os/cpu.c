#include "cpu.h"
#include "riscv.h"

struct cpu cpus[NCPU];

void wait_all_halt()
{
    for (int i = 0; i < NCPU; i++)
    {
        while (!halted[i])
            ;
    }
}

volatile int booted[NCPU];
volatile int halted[NCPU];

void halt()
{
    uint64 hartid = r_tp();
    halted[hartid] = 1;
}

void init_cpu()
{
    for (int i = 0; i < NCPU; i++)
    {
        cpus[i].proc = NULL;
        cpus[i].noff = 0;
        cpus[i].maintence = 0;
        cpus[i].core_id = i;
    }
}