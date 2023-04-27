OUTPUT_ARCH(i386:x86-64)
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain)

ld_kernel_start = 0xFFFFFFFF80000000;

SECTIONS
{
    . = ld_kernel_start;

    .text : {
        *(.text .text.*)
    }

    .data : {
        *(.data .data.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    }

    ld_kernel_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note .note.*)
    }
}
