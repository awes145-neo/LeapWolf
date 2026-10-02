/* linker script based on the DMPSTER one */

OUTPUT_FORMAT(elf32-littlearc)

MEMORY {
    RAM (rwx) : ORIGIN = 0x3C800000, LENGTH = 7M
}

PHDRS {
    image PT_LOAD FLAGS(7);
}

SECTIONS {
    .text (ORIGIN(RAM) + 16K) : {
        *(.text .text.*)
    } >RAM :image

    .rodata : {
        *(.rodata .rodata.*)
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array .ctors))
        __init_array_end = .;
    } >RAM :image

    .data : {
        *(.data .data.*)
        *(.sdata .sdata.* .sbss .sbss.*)
        /* bss stored as zeros in the file */
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
        __image_end = .;
    } >RAM :image

    /DISCARD/ : {
        *(.eh_frame .eh_frame_hdr .comment .note.* .ARC.attributes)
        *(.fini_array .fini_array.* .dtors)
    }
}
