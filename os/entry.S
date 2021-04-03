    .section .text.entry
    .globl _entry
_entry:
    # a0: hartid
    la sp, boot_stack
    li t1, 1024*16   # t1 = 16KB
    addi t0, a0, 1  # t0 = hartid + 1
    mul t0, t0, t1  # 16K * (hartid + 1)
    add sp, sp, t0  
    call main

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 1024 * 16 * 4
    .globl boot_stack_top
boot_stack_top:
