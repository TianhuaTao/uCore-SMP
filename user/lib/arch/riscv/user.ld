OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS {
    . = 0x1000;

    .startup : {
        *crt.S.o(.text)
    }

    .text : { *(.text) }
    .data : { *(.data .rodata) }

    /DISCARD/ : { *(.eh_*) }
}