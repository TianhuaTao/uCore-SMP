#include "defs.h"
#include "trap.h"
#include "proc.h"
#include "ucore.h"
#include "memory_layout.h"
static int app_cur, app_num;
static uint64 *app_info_ptr;
extern char _app_num[], _app_names[];
const uint64 BASE_ADDRESS = 0x1000; // user text start
char names[20][100];


void batchinit() {
    char* s;
    app_info_ptr = (uint64 *) _app_num;
    app_cur = -1;
    app_num = *app_info_ptr;
    app_info_ptr++;

    s = _app_names;
    for(int i = 0; i < app_num; ++i) {
        int len = strlen(s);
        strncpy(names[i], (const char*)s, len);
        s += len + 1;
        tracef("new name: %s\n", names[i]);
    }
}

int get_id_by_name(char* name) {
    for(int i = 0; i < app_num; ++i) {
        if(strncmp(name, names[i], 100) == 0)
            return i;
    }
    warnf("not find such app\n");
    return -1;
}

void alloc_ustack(struct proc *p) {
    if (mappages(p->pagetable, USTACK_BOTTOM, USTACK_SIZE, (uint64) kalloc(), PTE_U | PTE_R | PTE_W | PTE_X) != 0) {
        panic("alloc_ustack");
    }
    p->ustack = USTACK_BOTTOM;
    p->trapframe->sp = p->ustack + USTACK_SIZE;
    if(p->trapframe->sp > BASE_ADDRESS) {
        panic("error memory_layout");
    }
}


void bin_loader(uint64 start, uint64 end, struct proc *p) {
    infof("load range = [%p, %p)\n", start, end);
    uint64 s = PGROUNDDOWN(start), e = PGROUNDUP(end), length = e - s;
    for(uint64 va = BASE_ADDRESS, pa = s; pa < e; va += PGSIZE, pa += PGSIZE) {
        void* page = kalloc();
        if(page == 0) {
            panic("bin_loader kalloc");
        }
        memmove(page, (const void*)pa, PGSIZE);
        if (mappages(p->pagetable, va, PGSIZE, (uint64)page, PTE_U | PTE_R | PTE_W | PTE_X) != 0)
            panic("bin_loader mappages");
    }

    p->trapframe->epc = BASE_ADDRESS;
    alloc_ustack(p);
    p->sz = USTACK_SIZE + length;
}


void loader(int id, void* p) {
    infof("loader %s\n", names[id]);
    bin_loader(app_info_ptr[id], app_info_ptr[id+1], p);
}

int run_all_app() {
    struct proc *p = allocproc();
    p->parent = 0;
    int id = get_id_by_name("user_shell");
    if(id < 0)
        panic("no user shell");
    loader(id, p);
    p->state = RUNNABLE;
    release(&p->lock);
    return 0;
}
