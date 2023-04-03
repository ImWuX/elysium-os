# Important Things To Not Forget
- Kernel has not moved stack from the bootloader provided 64kb safe stack

# Backlog
Kernel:
  - Probably stop assuming that uninitialized variables are zero. cuz yk UB
  - Mouse driver seems to fail if mouse is moving on initialization - todo with weird ps2 bs
  - Check that the PAT is actually setup properly
  - Map framebuffer as write combining and memory mapped HW as non cacheable
  - Claim bootloader reclaimable
  - Kernel logging system
  - kernel/cpu/interrupts.asm: Dont do cli in interrupt handlers???
  - Unmap memory
    - Invalidate pages
  - Device Manager
  - Optimize VESA
  - **Devices**
    - CMOS
    - Serial?
    - HPET
  - Processes / context switching
  - SMP
  - TIMERS
    - get_clock_source()
      - if (invariant tsc supported && lapic supports invariant tsc) use invariant tsc
      - if (hpet supported) use hpet
      - if (lapic supported) use lapic timer
    - calibration_target()
      - if (cpuid has tsc freq) calibrate against invariant tsc
      - if (hpet supported) calibrate against hpet 
      - if (pit supported) calibrate against pit
  - smap/smep

Userspace:
  - Roll [MLibc](https://github.com/managarm/mlibc)

# Bugs
- AHCI assumes that every port has 32 command slots
- AHCI & Fat32 & Bootsector assumes 512byte sectors

# Possible Optimizations
- GCC O3 flag?
- Enable mmx, sse, sse2 (Currently gcc is told not to use these instructions)

# Documentation
Commands for creating the empty.img images are the following:
- 64MiB: `dd if=/dev/zero of=build/empty.img bs=512 count=131072`
- 512MiB: `dd if=/dev/zero of=build/empty.img bs=512 count=1048576`
- 5GiB: `dd if=/dev/zero of=build/empty.img bs=512 count=10485760`

## Virtual Memory Structure
|Start Address (Hex)|End Address (Hex)|Description|
|-|-|-|
|0000 0000 0000 0000|0000 7FFF FFFF FFFF|Process Address Space|
|FFFF 8000 0000 0000|FFFF FEFF FFFF FFFF #NF|Higher Half Direct Map|
|FFFF FF00 0000 0000|FFFF FFFF 7FFF FFFF #NF|Kernel Heap|
|FFFF FFFF 8000 0000|FFFF FFFF FFFF FFFF #NF|Kernel|

#NF = Not Enforced (As of now)