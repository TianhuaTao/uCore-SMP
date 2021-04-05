    .section .text.entry
    .globl _entry
_entry:
    # a0: hartid
    # every core has a boot stack of 4 KB
    la sp, boot_stack
    li t1, 1024*4   # t1 = 4KB
    addi t0, a0, 1  # t0 = hartid + 1
    mul t0, t0, t1  # 4K * (hartid + 1)
    add sp, sp, t0  
    call main

    .section .bss.stack
    .globl boot_stack
    .globl boot_stack_top
boot_stack:
    .space 1024 * 4 * 8
    .globl boot_stack_top
boot_stack_top:
