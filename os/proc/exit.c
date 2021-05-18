#include <proc/proc.h>

static void close_proc_files(struct proc *p) {
    // close files
    for (int i = 0; i < FD_MAX; ++i) {
        if (p->files[i] != NULL) {
            fileclose(p->files[i]);
            p->files[i] = NULL;
        }
    }
}

/**
 * Exit current running process
 */
void exit(int code) {
    struct proc *p = curr_proc();
    acquire(&p->lock);
    p->exit_code = code;

    close_proc_files(p);

    // TODO: recycle children

    if (p->parent != NULL) {
        p->state = ZOMBIE;
    } else {
        p->state = UNUSED;
        freeproc(p);
    }
    infof("proc %d exit with %d\n", p->pid, code);
    switch_to_scheduler();
}
