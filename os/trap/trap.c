#include <arch/riscv.h>
#include <mem/memory_layout.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <ucore/defs.h>
#include <ucore/ucore.h>

extern char trampoline[], uservec[], userret[];
void kernelvec();

void trapinit_hart() {
    set_kerneltrap();
    w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);
}

void trapinit() {
}

void unknown_trap() {
    printf("unknown trap: %p, stval = %p\n", r_scause(), r_stval());
    exit(-1);
}

// set up to take exceptions and traps while in the kernel.
void set_usertrap(void) {
    intr_off();
    w_stvec(((uint64)TRAMPOLINE + (uservec - trampoline)) & ~0x3); // DIRECT
}

void set_kerneltrap(void) {
    w_stvec((uint64)kernelvec & ~0x3); // DIRECT
    intr_on();
}

void kernel_interrupt_handler(uint64 cause) {
    int irq;
    switch (cause) {
    case SupervisorTimer:
        set_next_timer();
        // if form user, allow yield
        // TODO: enable yield
        break;
    case SupervisorExternal:
        irq = plic_claim();
        if (irq == UART0_IRQ) {
            infof("unexpected interrupt irq=UART0_IRQ");

        } else if (irq == VIRTIO0_IRQ) {
            virtio_disk_intr();
        } else if (irq) {
            infof("unexpected interrupt irq=%d", irq);
        }
        if (irq) {

            plic_complete(irq);
        }
        break;
    default:
        errorf("unknown kernel interrupt: %p, stval = %p\n", r_scause(), r_stval());
        panic("kernel interrupt");
        break;
    }
}

void user_interrupt_handler(uint64 cause) {
    int irq;
    switch (cause) {
    case SupervisorTimer:
        set_next_timer();
        // if form user, allow yield
        if ((r_sstatus() & SSTATUS_SPP) == 0) {
            yield();
        }
        break;
    case SupervisorExternal:
        irq = plic_claim();
        if (irq == UART0_IRQ) {
            infof("unexpected interrupt irq=UART0_IRQ");

        } else if (irq == VIRTIO0_IRQ) {
            virtio_disk_intr();
        } else if (irq) {
            infof("unexpected interrupt irq=%d", irq);
        }
        if (irq) {

            plic_complete(irq);
        }
        break;
    default:
        unknown_trap();
        break;
    }
}
void user_exception_handler(uint64 scause, uint64 stval, uint64 sepc) {
    struct trapframe *trapframe;
    switch (scause) {
    case UserEnvCall:
        trapframe = curr_proc()->trapframe;

        trapframe->epc += 4;
        intr_on();
        syscall();
        break;
    case StoreFault:
    case StorePageFault:
    case InstructionFault:
    case InstructionPageFault:
    case LoadFault:
    case LoadPageFault:
        infof(
            "scause = %d in application, bad addr = %p, bad instruction = %p, core dumped.\n",
            cause,
            r_stval(),
            trapframe->epc);
        exit(-2);
        break;
    case IllegalInstruction:
        infof("IllegalInstruction in application, core dumped.\n");
        exit(-3);
        break;
    default:
        unknown_trap();
        break;
    }
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap() {
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();
    uint64 stval = r_stval();

    KERNEL_ASSERT(!intr_get(), "");
    debugcore("Enter user trap handler scause=%p", scause);

    w_stvec((uint64)kernelvec & ~0x3); // DIRECT
    // debugcore("usertrap");
    // print_cpu(mycpu());

    KERNEL_ASSERT((sstatus & SSTATUS_SPP) == 0, "usertrap: not from user mode");

    if (scause & (1ULL << 63)) { // interrput = 1
        user_interrupt_handler(scause & 0xff);
    } else { // interrput = 0
        user_exception_handler(scause, stval, sepc);
    }
    usertrapret();
}

//
// return to user space
//
void usertrapret() {
    debugcore("About to return to user mode");

    // print_cpu(mycpu());
    // we're about to switch the destination of traps from
    // kerneltrap() to usertrap(), so turn off interrupts until
    // we're back in user space, where usertrap() is correct.
    // intr_off();
    set_usertrap();
    // print_cpu(mycpu());
    struct trapframe *trapframe = curr_proc()->trapframe;
    trapframe->kernel_satp = r_satp();                   // kernel page table
    trapframe->kernel_sp = curr_proc()->kstack + PGSIZE; // process's kernel stack
    trapframe->kernel_trap = (uint64)usertrap;
    trapframe->kernel_hartid = r_tp(); // hartid for cpuid()
    // debugf("epc=%p",trapframe->epc);
    w_sepc(trapframe->epc);
    // set up the registers that trampoline.S's sret will use
    // to get to user space.

    // set S Previous Privilege mode to User.
    uint64 x = r_sstatus();
    x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
    x |= SSTATUS_SPIE; // enable interrupts in user mode
    w_sstatus(x);

    // tell trampoline.S the user page table to switch to.
    uint64 satp = MAKE_SATP(curr_proc()->pagetable);

    // jump to trampoline.S at the top of memory, which
    // switches to the user page table, restores user registers,
    // and switches to user mode with sret.
    uint64 fn = TRAMPOLINE + (userret - trampoline);
    // debugcore("return to user, satp=%p, trampoline=%p, kernel_trap=%p\n",satp, fn,  trapframe->kernel_trap);
    ((void (*)(uint64, uint64))fn)(TRAPFRAME, satp);
}

void kernel_exception_handler(uint64 scause, uint64 stval, uint64 sepc) {
    errorf("invalid trap from kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
    panic("kernel exception");
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void kerneltrap() {
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();
    uint64 stval = r_stval();

    KERNEL_ASSERT(!intr_get(), "Interrupt can not be turned on in trap handler");
    KERNEL_ASSERT((sstatus & SSTATUS_SPP) != 0, "kerneltrap: not from supervisor mode");
    debugcore("Enter kernel trap handler, scause=%p", scause);

    if (scause & (1ULL << 63)) // interrput
    {
        kernel_interrupt_handler(scause & 0xff);
    } else // exception
    {
        kernel_exception_handler(scause, stval, sepc);
    }

    // the yield() may have caused some traps to occur,
    // so restore trap registers for use by kernelvec.S's sepc instruction.
    w_sepc(sepc);
    w_sstatus(sstatus);
    debugcore("About to return from kernel trap handler");

    // go back to kernelvec.S
}
