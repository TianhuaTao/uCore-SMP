OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS {
    
    .text : { *(.text.entry) *(.text) }
    .data : { *(.data .rodata) }

    /DISCARD/ : { *(.eh_*) }
}
