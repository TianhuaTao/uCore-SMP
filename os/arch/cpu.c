#include <arch/cpu.h>
#include <arch/riscv.h>

struct cpu cpus[NCPU];
// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void) {
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid() {
    int id = r_tp();
    return id;
}

void wait_all_halt() {
    for (int i = 0; i < NCPU; i++) {
        while (!halted[i])
            ;
    }
}

volatile int booted[NCPU];
volatile int halted[NCPU];

/**
 * set halted of this core to True
 */
void halt() {
    uint64 hartid = r_tp();
    halted[hartid] = TRUE;
}

void init_cpu() {
    for (int i = 0; i < NCPU; i++) {
        cpus[i].proc = NULL;
        cpus[i].noff = 0;
        cpus[i].base_interrupt_status = 0;
        cpus[i].core_id = i;
    }
}

/**
 * @brief For debugging.
 * 
 * @param c 
 */
void print_cpu(struct cpu *c) {
    printf_k("* ---------- CPU INFO ----------\n");
    printf_k("* core id: %d\n", c->core_id);
    printf_k("* interrupt before: %d\n", c->base_interrupt_status);
    printf_k("* num push off: %d\n", c->noff);
    printf_k("* proc: %p\n", c->proc);
    printf_k("* -------------------------------\n\n");
}