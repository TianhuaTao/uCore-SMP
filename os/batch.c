#include "defs.h"
#include "trap.h"
#include "proc.h"
#include "ucore.h"
static int app_cur, app_num;
static uint64 *app_info_ptr;
extern char _app_num[], ekernel[];
const uint64 BASE_ADDRESS = 0x80400000, MAX_APP_SIZE = 0x20000;

void batchinit()
{
    if ((uint64)ekernel >= BASE_ADDRESS)
    {
        panic("kernel too large...\n");
    }
    app_info_ptr = (uint64 *)_app_num;
    app_cur = -1;
    app_num = *app_info_ptr;
    app_info_ptr++;
}

int load_app(int n, uint64 *info)
{
    uint64 start = info[n], end = info[n + 1], length = end - start;
    memset((void *)BASE_ADDRESS + n * MAX_APP_SIZE, 0, MAX_APP_SIZE);
    memmove((void *)BASE_ADDRESS + n * MAX_APP_SIZE, (void *)start, length);
    return length;
}

int run_all_app()
{
    for (int i = 0; i < app_num; ++i)
    {
        struct proc *p = allocproc();
        struct trapframe *trapframe = p->trapframe;
        infof("run app %d\n", i);
        load_app(i, app_info_ptr);
        uint64 entry = BASE_ADDRESS + i * MAX_APP_SIZE;
        p->entry = entry;

        trapframe->epc = entry;
        trapframe->sp = (uint64)p->ustack + PAGE_SIZE;
        p->state = RUNNABLE;
    }
    return 0;
}