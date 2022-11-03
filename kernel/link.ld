ENTRY(kmain)
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ld_kernel_start = 0xFFFFFFFF80000000;

SECTIONS
{
    . = ld_kernel_start;

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .rodata : {
        *(.rodata)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }

    ld_kernel_end = .;

    /DISCARD/ : {
        *(.comment)
    }
}
