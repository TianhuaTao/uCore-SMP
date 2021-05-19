#include <arch/riscv.h>
#include <ucore/ucore.h>
#include <file/file.h>
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
static void clean_bss() {
    char *p;
    for (p = s_bss; p < e_bss; ++p)
        *p = 0;
}

volatile static int first_hart = 1;
volatile static int all_started = 0;
void start_hart(uint64 hartid, uint64 start_addr, uint64 a1);
void hart_bootcamp(uint64 hartid, uint64 a1) {
    w_tp(hartid);
    kvminithart(); // turn on paging
    trapinit_hart();
    plicinithart(); // ask PLIC for device interrupts

    printf("[ucore] start bootcamp hart %d\n", hartid);
    booted[hartid] = 1;
}

void wait_all_boot() {
    for (int i = 0; i < NCPU; i++) {
        while (!booted[i])
            ;
    }
    all_started = 1;
}
void init_booted() {
    for (int i = 0; i < NCPU; i++) {
        booted[i] = 0;
        halted[i] = 0;
    }
}

void test_misaligned(void)
{
    char arr[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    printf_k("arr: %p\n", (uint64)arr);
    printf_k("load half: ");
    uint64 res;
    for (int i = 0; i < 2; ++i)
    {
        uint64 addr = ((uint64)arr) + i;
        asm("lh  %0, 0(%1)"
            : "=r"(res)
            : "r"(addr));
        printf_k("%x  ", res);
    }
    printf_k("\n");

    printf_k("load word: ");
    for (int i = 0; i < 4; ++i)
    {
        uint64 addr = ((uint64)arr) + i;
        asm("lw  %0, 0(%1)"
            : "=r"(res)
            : "r"(addr));
        printf_k("%x  ", res);
    }
    printf_k("\n");

    printf_k("load dword: ");
    for (int i = 0; i < 8; ++i)
    {
        uint64 addr = ((uint64)arr) + i;
        asm("ld  %0, 0(%1)"
            : "=r"(res)
            : "r"(addr));
        printf_k("%x  ", res);
    }
    printf_k("\n");

    printf_k("save half:\n");
    for (int i = 0; i < 2; ++i)
    {
        uint64 addr = ((uint64)arr) + i;
        uint16 val = 0x7BCD;
        asm("sh  %0, 0(%1)"
            :
            : "r"(val), "r"(addr));
        for (int j = 0; j < 16; ++j)
            printf_k("%x ", arr[j]);
        printf_k("\n");
    }

    printf_k("save word:\n");
    for (int i = 0; i < 4; ++i)
    {
        uint64 addr = ((uint64)arr) + i;
        uint32 val = 0xDEADBEEF;
        asm("sw  %0, 0(%1)"
            :
            : "r"(val), "r"(addr));
        for (int j = 0; j < 16; ++j)
            printf_k("%x ", arr[j]);
        printf_k("\n");
    }

    printf_k("save dword:\n");
    for (int i = 0; i < 8; ++i)
    {
        uint64 addr = ((uint64)arr) + i;
        uint64 val = 0xD00DDAD0BAD0DEEDULL;
        asm("sd  %0, 0(%1)"
            :
            : "r"(val), "r"(addr));
        for (int j = 0; j < 16; ++j)
            printf_k("%x ", arr[j]);
        printf_k("\n");
    }
    printf_k("\n");
    ebreak();
}

extern char _entry[];
void main(uint64 hartid, uint64 a1) {
    if (first_hart) {
        w_tp(hartid);
        first_hart = FALSE;
        // test_misaligned();
        printf_k("\n");
        printf_k("[ucore] Boot hartid=%d\n", hartid);
        printf_k("[ucore] Core count: %d\n", NCPU);
        printf_k("[ucore] a1=%p\n", a1);
        printf_k("[ucore] s_text=%p, e_text=%p\n", s_text, e_text);
        printf_k("[ucore] s_rodata=%p, e_rodata=%p\n", s_rodata, e_rodata);
        printf_k("[ucore] s_data=%p, e_data=%p\n", s_data, e_data);
        printf_k("[ucore] s_bss_stack=%p, e_bss_stack=%p\n", boot_stack, boot_stack_top);
        printf_k("[ucore] s_bss=%p, e_bss=%p\n", s_bss, e_bss);

        clean_bss();
        init_cpu();
        printfinit();
        trapinit();
        trapinit_hart();
        kinit();
        procinit();
        plicinit();     // set up interrupt controller
        plicinithart(); // ask PLIC for device interrupts
        binit();        // buffer cache
        inode_table_init();        // inode cache
        fileinit();     // file table
        // virtio_disk_init();
        init_abstract_disk();
        kvminit();
        kvminithart();
        timerinit();    // do nothing
        init_app_names();
        init_scheduler();
        make_shell_proc();

        init_booted();
        booted[hartid] = 1;

        // measure time
        // for (int i = 0; i < 20; i++)
        // {
        //     uint64 start = r_time();
        //     while (1)
        //     {
        //         uint64 cycle = r_time();
        //         uint64 delta = cycle- start ;
        //         if(delta>10000000){
        //             printf_k("%d %p %p %d\n",i, cycle,delta, r_cycle());
        //             break;
        //         }
        //     }
        // }


        for (int i = 0; i < NCPU; i++) {
            if (i != hartid) // not this hart
            {
                printf("[ucore] start hart %d\n", i);
                start_hart(i, (uint64)_entry, 0);
            }
        }

        // ebreak();
        wait_all_boot();
    } else {
        hart_bootcamp(hartid, a1);
    }
    while (!all_started) {
        ; // wait until all hard started
    }

    debugcore("start scheduling!");
    scheduler();
    debugf("halt");
    halt();
    if (cpuid() == 0) {
        debugcore("wait other halt");
        wait_all_halt();
        printf("[ucore] All finished. Shutdown ...\n");
        shutdown();
    } else {
        for (;;)
            ;
    }
}