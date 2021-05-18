#include <proc/proc.h>

// int spawn(char *filename) {
//     int pid;
//     struct proc *np;
//     struct proc *p = curr_proc();

//     // Allocate process.
//     if ((np = allocproc()) == 0) {
//         panic("allocproc\n");
//     }
//     // info("alloc\n");
//     // Copy user memory from parent to child.
//     if (uvmcopy(p->pagetable, np->pagetable, p->total_size) < 0) {
//         panic("uvmcopy\n");
//     }
//     np->total_size = p->total_size;

//     // copy saved user registers.
//     *(np->trapframe) = *(p->trapframe);

//     // Cause fork to return 0 in the child.
//     np->trapframe->a0 = 0;
//     pid = np->pid;
//     np->parent = p;
//     np->state = RUNNABLE;

//     // info("fork done\n");

//     char name[200];
//     copyinstr(p->pagetable, name, (uint64)filename, 200);
//     infof("sys_exec %s\n", name);

//     int id = get_app_id_by_name(name);
//     // info("id=%d\n", id);
//     if (id < 0)
//         return -1;
//     // info("free\n");
//     proc_free_mem_and_pagetable(np->pagetable, np->total_size);
//     np->total_size = 0;
//     np->pagetable = proc_pagetable(np);
//     if (np->pagetable == 0) {
//         panic("");
//     }
//     // info("load\n");
//     loader(id, np);
//     release(&np->lock);
//     return pid;
// }
