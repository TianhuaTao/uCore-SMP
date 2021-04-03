#include "ucore.h"
#include "riscv.h"
extern char ekernel[];
extern char s_bss[];
extern char e_bss[];
void clean_bss()
{
    char *p;
    for (p = s_bss; p < e_bss; ++p)
        *p = 0;
}

volatile static int first_hart = 1;
void start_hart(uint64 hartid, uint64 start_addr, uint64 a1);
void hart_bootcamp(uint64 hartid, uint64 a1)
{
    w_tp(hartid);
    trapinit();
    timerinit();
    printf("[ucore] start bootcamp hart %d\n", hartid);
    booted[hartid] = 1;
    // for (;;)
    //     ;
}

void wait_all_boot()
{
    for (int i = 0; i < NCPU; i++)
    {
        while (!booted[i])
            ;
    }
}
void init_booted()
{
    for (int i = 0; i < NCPU; i++)
    {
        booted[i] = 0;
        halted[i] = 0;
    }
}

extern char _entry[];
void main(uint64 hartid, uint64 a1)
{
    if (first_hart)
    {
        w_tp(hartid);

        first_hart = 0;
        printf("\n");
        printf("[ucore] Boot hartid=%d\n", hartid);
        printf("[ucore] a1=%d\n", a1);

        clean_bss();
        init_cpu();
        printfinit();
        trapinit();
        batchinit();
        procinit();
        timerinit();
        init_scheduler();
        run_all_app();
        init_booted();
        booted[hartid] = 1;

        for (int i = 0; i < NCPU; i++)
        {
            if (i != hartid)
            {
                printf("[ucore] start hart %d\n", i);
                start_hart(i, (uint64)_entry, 0);
            }
        }

        wait_all_boot();
    }
    else
    {
        hart_bootcamp(hartid, a1);
    }
    tracef("start scheduler!");
    scheduler();
    debugf("core %d halt", cpuid());
    halt();
    if (cpuid() == 0)
    {
        debugcore("wait other halt");
        wait_all_halt();
        printf("[ucore] All finished. Shutdown ...\n");
        shutdown();
    }
    else
    {
        for (;;)
            ;
    }
}