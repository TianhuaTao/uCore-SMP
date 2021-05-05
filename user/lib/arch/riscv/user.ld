OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS {
    . = 0x1000;

    .startup : {
        *crt.S.o(.text)
    }

    .text : { 
        *(.text .text.*)
    }

    .bss : { 
        *(.bss .bss.*)
        *(.sbss .sbss.*)
    }

    .data : { 
        *(.data .rodata) 
        *(.sdata .sdata.*)
    }
    
    /DISCARD/ : { *(.eh_*) }
}