#include "defs.h"
#include "trap.h"
#include "proc.h"
#include "ucore.h"
#include "memory_layout.h"
static int app_cur, app_num;
static uint64 *app_info_ptr;
extern char _app_num[];
const uint64 BASE_ADDRESS = 0x1000; // user text start

void batchinit()
{
    app_info_ptr = (uint64 *)_app_num;
    app_cur = -1;
    app_num = *app_info_ptr;
}

pagetable_t bin_loader(uint64 start, uint64 end, struct proc *p)
{
    pagetable_t pg = uvmcreate();
    p->trapframe = (struct trapframe *)kalloc();
    memset(p->trapframe, 0, PGSIZE);

    if (mappages(pg, TRAPFRAME, PGSIZE,
                 (uint64)p->trapframe, PTE_R | PTE_W) < 0)
    {
        panic("mappages fail\n");
    }
    debugf("TRAPFRAME va=%p -> [%p, %p]\n", TRAPFRAME, (uint64)p->trapframe, ((uint64)p->trapframe) + PGSIZE);

    uint64 s = PGROUNDDOWN(start), e = PGROUNDUP(end);

    if (mappages(pg, BASE_ADDRESS, e - s, s, PTE_U | PTE_R | PTE_W | PTE_X) != 0)
    {
        panic("wrong loader 1\n");
    }
    debugf("bin start va=%p -> [%p, %p]\n", BASE_ADDRESS, s, e);

    p->pagetable = pg;
    p->trapframe->epc = BASE_ADDRESS;
    uint64 ustack_pa = (uint64)kalloc();
    uint64 ustack_va = USTACK_BOTTOM;
    mappages(pg, ustack_va, USTACK_SIZE, ustack_pa, PTE_U | PTE_R | PTE_W | PTE_X);
    debugf("ustack va=%p -> [%p, %p]\n", ustack_va, ustack_pa, USTACK_SIZE + ustack_pa);

    p->ustack = ustack_va;
    p->trapframe->sp = p->ustack + USTACK_SIZE;
    if (p->trapframe->sp > BASE_ADDRESS)
    {
        panic("error memory_layout");
    }
    // debugf("return bin_loader");
    return pg;
}

int run_all_app()
{
    for (int i = 0; i < app_num; ++i)
    {
        app_cur++;
        app_info_ptr++;
        struct proc *p = allocproc();
        debugf("after allocproc");
        debugf("load app %d", app_cur);
        bin_loader(app_info_ptr[0], app_info_ptr[1], p);
        p->state = RUNNABLE;
    }
    return 0;
}