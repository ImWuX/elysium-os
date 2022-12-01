# Important Things To Not Forget
- Stack is still at initial
- Boot sector doesnt have to store the values like it does because the ahci driver doesn't use them

# Backlog
Current:
  - [ ] ELF loader should create the virtual memory mappings for the file
  - [ ] Bootloader should discard itself along with any data not needed
  - [ ] Bootloader should pass simple map to kernel

Kernel:
  - Memory is being identity mapped all over the fucking place. HHDM!!!
    - Proper way to map memory to HHDM; AND ACCESS
  - kmalloc over HHDM?
  - Some Type of kernel error system
  - Kernel logging system... we have log.c but it isnt used anywhere and its shit
  - kernel/cpu/interrupts.asm: Dont do cli in interrupt handlers???
  - kernel/cpu/interrupts.asm: Possibly want to also set SS register to data descriptor?
  - Unmap memory
    - Invalidate pages
  - Device Manager
  - Optimize VESA
  - Probably use the MSR for the APIC address instead of relying on ACPI tables?
  - **Devices**
    - CMOS
    - Serial?
    - HPET
  - Processes / context switching
  - SMP

Bootloader:
  - Elf loading is super rudamentary
  - VESA 2.0 is assumed. Not checked...
  - Right now the display mode is just picked if it is 1920x1080 and rgb. It should be more dynamic and shit
  - Rename MBR

# Bugs
- AHCI assumes that every port has 32 command slots
- Stage 1 of bootloader probably doesnt map enough memory to map 512tb of ram lol
- Bootloader only supports 512byte sectors, technically fat32 supports other sizes.

# Assumptions Made
Bootloader:
- Assumes that there is conventional memory at 0x8000 up for the bootloader memory map
- Assumes that there is conventional memory at 0x500 for the bios memory map
- Assumes that there is conventional memory at 0x50000 for the initial paging structure

# Documentation
- MBR Errors
  - `E:0` means a disk error occured
  - `E:1` means the bootloader was not found.

Command for creating the empty.img image is the following:
64MiB: `dd if=/dev/zero of=out/empty.img bs=512 count=131072`
512MiB: `dd if=/dev/zero of=out/empty.img bs=512 count=1048576`
5GiB: `dd if=/dev/zero of=out/empty.img bs=512 count=10485760`

## Physical Memory Structure
- B1 Page Structure         0x50000 ⬆️
- Bootloader Stage 2        0x8000 ⬆️
- Bootloader Stage 1 Buffer 0x7E00 ➡️ 0x8000
- Bootloader Stage 1        0x7C00 ➡️ 0x7E00
- Bootloader Stage 1 Stack  0x7C00 ⬇️

## Virtual Memory Structure
- Kernel at                 0xFFFF FFFF 8000 0000 ⬆️
- HHDM at                   0xFFFF 8000 0000 0000 ⬆️ (//TODO: HHDM Might run into kernel lol)
- Heap mapped at            0x0000 1000 0000 0000 ⬆️
- First 2mb identity mapped 0x0000 0000 0000 0000 ➡️ 0x0000 0000 0000 0002 0000