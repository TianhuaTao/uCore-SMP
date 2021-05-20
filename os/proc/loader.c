#include <ucore/defs.h>
#include <mem/memory_layout.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <ucore/ucore.h>
static int app_cur, app_num;
static uint64 *app_info_ptr;
extern char _app_num[], _app_names[];
// const uint64 BASE_ADDRESS = 0x1000; // user text start
#define APP_NAME_MAX 100
#define APP_MAX_CNT 40
char names[APP_MAX_CNT][APP_NAME_MAX];

void init_app_names()
{
    char *s;
    app_info_ptr = (uint64 *)_app_num;
    app_cur = -1;
    app_num = *app_info_ptr;
    app_info_ptr++;

    s = _app_names;
    for (int i = 0; i < app_num; ++i)
    {
        int len = strlen(s);
        strncpy(names[i], (const char *)s, len);
        s += len + 1;
        tracef("app name: %s", names[i]);
    }
}

int get_app_id_by_name(char *name)
{
    for (int i = 0; i < app_num; ++i)
    {
        if (strncmp(name, names[i], APP_NAME_MAX) == 0)
            return i;
    }
    return -1;
}

void alloc_ustack(struct proc *p)
{
    if (mappages(p->pagetable, USER_STACK_BOTTOM - USTACK_SIZE, USTACK_SIZE, (uint64)alloc_physical_page(), PTE_U | PTE_R | PTE_W | PTE_X) != 0)
    {
        panic("alloc_ustack");
    }
    p->ustack_bottom = USER_STACK_BOTTOM;
    p->trapframe->sp = p->ustack_bottom;
}

void bin_loader(uint64 start, uint64 end, struct proc *p)
{
    debugf("load range = [%p, %p)", start, end);
    uint64 s = PGROUNDDOWN(start), e = PGROUNDUP(end), length = e - s;
    for (uint64 va = USER_TEXT_START, pa = s; pa < e; va += PGSIZE, pa += PGSIZE)
    {
        void *page = alloc_physical_page();
        if (page == NULL)
        {
            panic("bin_loader alloc_physical_page");
        }
        memmove(page, (const void *)pa, PGSIZE);
        if (mappages(p->pagetable, va, PGSIZE, (uint64)page, PTE_U | PTE_R | PTE_W | PTE_X) != 0)
            panic("bin_loader mappages");
    }

    p->trapframe->epc = USER_TEXT_START;
    alloc_ustack(p);
    p->total_size = USTACK_SIZE + length;
}

void loader(int id, struct proc *p) {
    // infocore("loader %s", names[id]);
    bin_loader(app_info_ptr[id], app_info_ptr[id + 1], p);
    mmiowb();
}

// load shell from kernel data section
// and make it a proc
int make_shell_proc()
{
    struct proc *p = alloc_proc();

    // still need to init:
    //  * parent
    //  * ustack_bottom
    //  * total_size
    //  * cwd
    //  * name

    // parent
    p->parent = NULL;

    int id = get_app_id_by_name( "shell" );
    if (id < 0)
        panic("no user shell");
    loader(id, p);  // will fill ustack_bottom and total_size

    // name
    safestrcpy(p->name, "shell", PROC_NAME_MAX);

    // cwd
    p->cwd = inode_by_name("/");
    p->state = RUNNABLE;
    release(&p->lock);

    return 0;
}
