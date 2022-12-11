# Nest Boot Protocol
The Nest Boot Protocol describes the state that the Nest bootloader will leave the kernel in. Note that the Nest bootloader is purely a x86_64 bootloader.

## The State
- A20 line is opened
- CPU is set to long mode
- HHDM is mapped into virtual memory
    - Starting at offset `0xFFFF800000000000`
    - Only usable entries are mapped

## The Memory Map
- Anything under `0x500` will always be `RESERVED`.
- All entries are ensured to not be overlapping.
- All entries are ensured to be ordered.
- Contigous memory of the same type is ensured to be one entry. (No back to back entries of the same type)
- `USABLE` and `BOOT_RECLAIMABLE` entries are ensured to be:
    - Page aligned (`0x1000`)
- `BOOT_RECLAIMABLE` entries should only be used once the kernel is either done with data provided by the bootloader or has moved it.