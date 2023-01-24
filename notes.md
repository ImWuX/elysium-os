# Important Things To Not Forget
- Stack is still at initial

# Backlog
Kernel:
  - Memory is being identity mapped all over the fucking place. HHDM!!!
    - Proper way to map memory to HHDM; AND ACCESS
  - kmalloc over HHDM?
  - Claim bootloader reclaimable
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

Userspace:
  - Roll [MLibc](https://github.com/managarm/mlibc)

# Bugs
- AHCI assumes that every port has 32 command slots
- AHCI & Fat32 & Bootsector assumes 512byte sectors

# Possible Optimizations
- Not using mcmodel=large, not sure if this is possible
- Enable mmx, sse, sse2 (Currently gcc is told not to use these instructions)

# Documentation
Command for creating the empty.img image is the following:
64MiB: `dd if=/dev/zero of=build/empty.img bs=512 count=131072`
512MiB: `dd if=/dev/zero of=build/empty.img bs=512 count=1048576`
5GiB: `dd if=/dev/zero of=build/empty.img bs=512 count=10485760`

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