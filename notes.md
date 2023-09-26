# Important Things To Not Forget
- Kernel has not moved stack from the bootloader provided 64kb safe stack // this can be release at the userspace handoff

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