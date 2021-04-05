#include "ucore.h"
#include "riscv.h"

extern char s_bss[];
extern char e_bss[];
extern char s_text[];
extern char e_text[];
extern char s_rodata[];
extern char e_rodata[];
extern char s_data[];
extern char e_data[];
extern char boot_stack[];
extern char boot_stack_top[];
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
    kvminithart(); // turn on paging
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
        printf("[ucore] s_text=%p, e_text=%p\n", s_text, e_text);
        printf("[ucore] s_rodata=%p, e_rodata=%p\n", s_rodata, e_rodata);
        printf("[ucore] s_data=%p, e_data=%p\n", s_data, e_data);
        printf("[ucore] s_bss_stack=%p, e_bss_stack=%p\n", boot_stack, boot_stack_top);
        printf("[ucore] s_bss=%p, e_bss=%p\n", s_bss, e_bss);

        clean_bss();
        init_cpu();
        printfinit();
        trapinit();
        kinit();
        printf("kinit\n");
        procinit();

        kvminit();
        kvminithart();
        batchinit();
        printf("batchinit\n");
        timerinit();
        printf("timerinit\n");
        init_scheduler();
        printf("init_scheduler\n");
        run_all_app();
        printf("run_all_app\n");
        init_booted();
        debugf("INIT BOOT");
        booted[hartid] = 1;

        for (int i = 0; i < NCPU; i++)
        {
            if (i != hartid) // not this hart
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