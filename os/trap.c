#include "defs.h"
#include "trap.h"
#include "proc.h"
#include "riscv.h"
#include "ucore.h"
#include "memory_layout.h"
extern char trampoline[], uservec[];
extern char trampoline[], uservec[], userret[];


// set up to take exceptions and traps while in the kernel.
void trapinit(void)
{
   set_kerneltrap();
}

void unknown_trap() {
    printf("unknown trap: %p, stval = %p\n", r_scause(), r_stval());
    exit(-1);
}


void kerneltrap() {
    if((r_sstatus() & SSTATUS_SPP) == 0)
        panic("kerneltrap: should not handle user traps");

    uint64 cause = r_scause();
    errorf("[core%d] scause=%d in application, stval=%p, sepc=%p, core dumped.\n",
    cpuid(), cause,
                    r_stval(), r_sepc());
    panic("trap from kernel\n");
}

// set up to take exceptions and traps while in the kernel.
void set_usertrap(void) {
        w_stvec(((uint64) TRAMPOLINE + (uservec - trampoline)) & ~0x3); // DIRECT
}

void set_kerneltrap(void) {
    w_stvec((uint64)kerneltrap & ~0x3); // DIRECT
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap() {
    set_kerneltrap();
    // debugf("begin usertrap");
    struct trapframe *trapframe = curr_proc()->trapframe;

    if ((r_sstatus() & SSTATUS_SPP) != 0)
        panic("usertrap: not from user mode");

    uint64 cause = r_scause();
    if(cause & (1ULL << 63)) {
        cause &= ~(1ULL << 63);
        switch(cause) {
        case SupervisorTimer:
            // infof("time interrupt!\n");
            set_next_timer();
            yield();
            break;
        default:
            unknown_trap();
            break;
        }
    } else {
        switch(cause) {
        case UserEnvCall:
            trapframe->epc += 4;
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
                    trapframe->epc
            );
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
    usertrapret();
}

//
// return to user space
//
void usertrapret() {
     // we're about to switch the destination of traps from
    // kerneltrap() to usertrap(), so turn off interrupts until
    // we're back in user space, where usertrap() is correct.
    intr_off();
    set_usertrap();
    struct trapframe *trapframe = curr_proc()->trapframe;
    trapframe->kernel_satp = r_satp();                   // kernel page table
    trapframe->kernel_sp = curr_proc()->kstack + PGSIZE;// process's kernel stack
    trapframe->kernel_trap = (uint64) usertrap;
    trapframe->kernel_hartid = r_tp();// hartid for cpuid()
    // debugf("epc=%p",trapframe->epc);
    w_sepc(trapframe->epc);
    // set up the registers that trampoline.S's sret will use
    // to get to user space.

    // set S Previous Privilege mode to User.
    uint64 x = r_sstatus();
    x &= ~SSTATUS_SPP;// clear SPP to 0 for user mode
    x |= SSTATUS_SPIE;// enable interrupts in user mode
    w_sstatus(x);

    // tell trampoline.S the user page table to switch to.
    uint64 satp = MAKE_SATP(curr_proc()->pagetable);

    // jump to trampoline.S at the top of memory, which 
    // switches to the user page table, restores user registers,
    // and switches to user mode with sret.
    uint64 fn = TRAMPOLINE + (userret - trampoline);
    // debugcore("return to user, satp=%p, trampoline=%p, kernel_trap=%p\n",satp, fn,  trapframe->kernel_trap);
    ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}