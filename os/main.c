#include <arch/riscv.h>
#include <ucore/ucore.h>
#include <file/file.h>
#include <proc/proc.h>
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
    for (int i = 1; i < NCPU; i++) {
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

extern char _entry[];
void main(uint64 hartid, uint64 a1) {
    if (first_hart) {
        w_tp(hartid);
        first_hart = FALSE;
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
        init_trace();
        // virtio_disk_init();
        init_abstract_disk();
        kvminit();
        infof("kernel vm created");
        kvminithart();
        infof("kernel vm enabled");
        timerinit();    // do nothing
        init_app_names();
        init_scheduler();
        make_shell_proc();

        init_booted();
        booted[hartid] = 1;

        if (hartid >= NCPU){
            panic("unexpected hartid");
        }
        int CPU_START=1;    // core 0 is not usable

        for (int i = CPU_START; i < NCPU; i++) {
            if (i != hartid && i!=0) // not this hart
            {
                printf("[ucore] start hart %d\n", i);
                start_hart(i, (uint64)_entry, 0);
                while (booted[i] == 0)
                {
                    // wait
                }
                
            }
        }

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