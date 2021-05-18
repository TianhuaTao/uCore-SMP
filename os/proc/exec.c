#include <proc/proc.h>
#include <trap/trap.h>
#include <utils/log.h>

static void debug_print_args(char *name, int argc, const char **argv) {
    tracecore("exec name=%s, argc=%d", name, argc);
    for (int i = 0; i < argc; i++) {
        tracecore("argv[%d]=%s", i, argv[i]);
    }
}

int exec(char *name, int argc, const char **argv) {
    debug_print_args(name, argc, argv);

    int id = get_app_id_by_name(name);
    if (id < 0)
        return -1;
    struct proc *p = curr_proc();

    proc_free_mem_and_pagetable(p);
    p->total_size = 0;
    p->pagetable = proc_pagetable(p);
    if (p->pagetable == 0) {
        panic("");
    }
    loader(id, p);
    safestrcpy(p->name, name, PROC_NAME_MAX);
    // push args
    char *sp = (char *)p->trapframe->sp;
    phex(sp);

    // sp itself is on the boundary hence not mapped, but sp-1 is a valid address.
    // we can calculate the physical address of sp
    // but can NOT access sp_pa
    char *sp_pa = (char *)(virt_addr_to_physical(p->pagetable, (uint64)sp - 1) + 1);

    char *sp_pa_bottom = sp_pa; // keep a record

    // the argv array (content of argv[i]) will be stored on ustack, at the very bottom
    // and the real string value of argv array (content of *argv[i]) will be stored after that
    sp_pa -= argc * sizeof(const char *);           // alloc space for argv
    const char **argv_start = (const char **)sp_pa; // user main()'s argv value (physical here)

    for (int i = 0; i < argc; i++) {
        int arg_len = strlen(argv[i]);
        sp_pa -= arg_len + 1; // alloc space for argv[i] string, including a null
        strncpy(sp_pa, argv[i], arg_len);
        sp_pa[arg_len] = '\0';                       // make sure null is there
        argv_start[i] = (sp_pa - sp_pa_bottom) + sp; // point argv[i] to here, use user space
    }

    p->trapframe->sp += sp_pa - sp_pa_bottom; // update user sp
    p->trapframe->a0 = (uint64)argc;

    int64 offset = (uint64)argv_start - (uint64)sp_pa_bottom; // offset of argv in user stack
    p->trapframe->a1 = (uint64)(sp + offset);

    // tracecore("trapframe->sp=%p", p->trapframe->sp);
    // tracecore("trapframe->a0=%p", p->trapframe->a0);
    // tracecore("trapframe->a1=%p", p->trapframe->a1);

    return 0;
}
