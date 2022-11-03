OUTPUT_FORMAT(binary)
ENTRY(ld_bootloader_start)

ld_mmap = 0x500;
ld_temporary_paging = 0x50000;
ld_bootloader_start = 0x8000;

SECTIONS
{
    . = ld_bootloader_start;

    .entry : {
        *(.entry)
    }

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }

    ld_vbe_info = .;
    . += 512;

    ld_vbe_mode_info = .;
    . += 256;

    ld_bootloader_end = .;
}
