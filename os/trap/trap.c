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
    w_sie(r_sie() | SIE_SEIE | SIE_SSIE);
}

void trapinit() {
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

void kernel_interrupt_handler(uint64 scause, uint64 stval, uint64 sepc) {
    uint64 cause = scause & 0xff;
    int irq;
    switch (cause) {
    case SupervisorTimer:
        set_next_timer();
        yield();
        break;
    case SupervisorExternal:
        irq = plic_claim();
        if (irq == VIRTIO0_IRQ) {
            virtio_disk_intr();
        } else if(irq>0) {
            warnf("unexpected interrupt irq=%d", irq);
        }
        if (irq) {
            plic_complete(irq);
        }
        break;
    default:
        errorf("unknown kernel interrupt: %p, sepc=%p, stval = %p\n", scause, sepc, stval);
        panic("kernel interrupt");
        break;
    }
}

void user_interrupt_handler(uint64 scause, uint64 stval, uint64 sepc) {
    int irq;
    switch (scause & 0xff) {
    case SupervisorTimer:
        set_next_timer();
        yield();
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
        infof("Unknown interrupt in user application: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        exit(-1);
        break;
    }
}
void user_exception_handler(uint64 scause, uint64 stval, uint64 sepc) {
    struct proc *p = curr_proc();
    struct trapframe *trapframe = p->trapframe;
    switch (scause & 0xff) {
    case UserEnvCall:
        if (p->killed)
            exit(-1);
        trapframe->epc += 4;
        intr_on();
        syscall();
        break;
    case StoreAccessFault:
        infof("StoreAccessFault in user application: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        exit(-8);
        break;
    case StorePageFault:
        infof("StorePageFault in user application: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        exit(-7);
        break;
    case InstructionAccessFault:
        infof("InstructionAccessFault in user application: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        exit(-6);
        break;
    case InstructionPageFault:
        infof("InstructionPageFault in user application: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        exit(-5);
        break;
    case LoadAccessFault:
        infof("LoadAccessFault in user application: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        exit(-4);
        break;
    case LoadPageFault:
        infof("LoadPageFault in user application: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        print_proc(curr_proc());
        exit(-2);
        break;
    case IllegalInstruction:
        errorf("IllegalInstruction in user application: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        exit(-3);
        break;
    default:
        errorf("Unknown exception in user application: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        exit(-1);
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
    // if ((scause & 0xFF) != SupervisorTimer)
    //     debugcore("Enter user trap handler scause=%p", scause);

    w_stvec((uint64)kernelvec & ~0x3); // DIRECT
    // debugcore("usertrap");
    // print_cpu(mycpu());

    if ((sstatus & SSTATUS_SPP) != 0)
    {
        debugcore("sepc: %p, scause: 0x%x, stval: %p, sstatus: 0x%x\n", sepc, scause, stval, sstatus);
        errorcore("sepc: %p, scause: 0x%x, stval: %p, sstatus: 0x%x", sepc, scause, stval, sstatus);
    }
    KERNEL_ASSERT((sstatus & SSTATUS_SPP) == 0, "usertrap: not from user mode");

    if (scause & (1ULL << 63)) { // interrput = 1
        user_interrupt_handler(scause, stval, sepc);
    } else { // interrput = 0
        user_exception_handler(scause, stval, sepc);
    }
    usertrapret();
}

//
// return to user space
//
void usertrapret() {
    // debugcore("About to return to user mode");

    // print_cpu(mycpu());
    // we're about to switch the destination of traps from
    // kerneltrap() to usertrap(), so turn off interrupts until
    // we're back in user space, where usertrap() is correct.
    intr_off();
    struct proc *p = curr_proc();
    struct trapframe *trapframe = p->trapframe;
    // if ((trapframe->scause & 0xFF) != SupervisorTimer)
    //     debugcore("About to return to user mode");

    trapframe->kernel_satp = r_satp();         // kernel page table
    trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
    trapframe->kernel_trap = (uint64)usertrap;
    trapframe->kernel_hartid = r_tp(); // hartid for cpuid()
    // if ((trapframe->scause & 0xFF) != SupervisorTimer)
    //     debugcore("epc=%p\n", trapframe->epc);
    w_sepc(trapframe->epc);
    // set up the registers that trampoline.S's sret will use
    // to get to user space.


    // tell trampoline.S the user page table to switch to.
    uint64 satp = MAKE_SATP(p->pagetable);

    // jump to trampoline.S at the top of memory, which
    // switches to the user page table, restores user registers,
    // and switches to user mode with sret.
    uint64 fn = TRAMPOLINE + (userret - trampoline);
    // if ((trapframe->scause & 0xFF) != SupervisorTimer && trapframe->a7 != 64 && trapframe->a7 != 63 && trapframe->a7 != 124 && trapframe->a7 != 153)
    // if ((trapframe->scause & 0xFF) != SupervisorTimer && (is_panic_addr(trapframe->epc) || is_panic_addr(trapframe->ra) || is_panic_addr(p->context.ra)))
    // {
    //     infocore("return to user, satp=%p, trampoline=%p, kernel_trap=%p, epc=%p", satp, fn, trapframe->kernel_trap, trapframe->epc);
    //     mmiowb();
    // }

    // set S Previous Privilege mode to User.
    uint64 x = r_sstatus();
    x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
    x |= SSTATUS_SPIE; // enable interrupts in user mode
    w_sstatus(x);
    set_usertrap();

    ((void (*)(uint64, uint64))fn)(TRAPFRAME, satp);
}

void kernel_exception_handler(uint64 scause, uint64 stval, uint64 sepc) {
    switch (scause & 0xff) {
    case InstructionMisaligned:
        errorf("InstructionMisaligned in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case InstructionAccessFault:
        errorf("SupervisorEnvCall in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case IllegalInstruction:
        errorf("IllegalInstruction in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case Breakpoint:
        errorf("Breakpoint in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case LoadMisaligned:
        errorf("LoadMisaligned in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case LoadAccessFault:
        errorf("LoadAccessFault in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case StoreMisaligned:
        errorf("StoreMisaligned in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case StoreAccessFault:
        errorf("StoreAccessFault in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case SupervisorEnvCall:
        errorf("SupervisorEnvCall in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case MachineEnvCall:
        errorf("MachineEnvCall in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case InstructionPageFault:
        errorf("InstructionPageFault in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case LoadPageFault:
        errorf("LoadPageFault in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    case StorePageFault:
        errorf("StorePageFault in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    default:
        errorf("Unknown exception in kernel: %p, stval = %p sepc = %p\n", scause, stval, sepc);
        break;
    }
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
    if ((scause & 0xFF) != SupervisorTimer || (sepc & 0x1076) == 0x1076)
        debugcore("Enter kernel trap handler, scause=%p, sepc=%p", scause, sepc);

    if (scause & (1ULL << 63)) // interrput
    {
        kernel_interrupt_handler(scause, stval, sepc);
    } else // exception
    {
        kernel_exception_handler(scause, stval, sepc);
    }

    // the yield() may have caused some traps to occur,
    // so restore trap registers for use by kernelvec.S's sepc instruction.
    w_sepc(sepc);
    w_sstatus(sstatus);
    if ((scause & 0xFF) != SupervisorTimer || (sepc & 0x1076) == 0x1076)
        debugcore("About to return from kernel trap handler");

    // go back to kernelvec.S
}
