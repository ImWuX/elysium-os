#include <arch/amd64/msr.h>
#include <arch/amd64/gdt.h>

#define MSR_EFER_SCE 1

extern void syscall_entry();

void syscall_init() {
    msr_write(MSR_EFER, msr_read(MSR_EFER) | MSR_EFER_SCE);
    msr_write(MSR_STAR, ((uint64_t) GDT_CODE_RING0 << 32) | ((uint64_t) (GDT_CODE_RING3 - 16) << 48));
    msr_write(MSR_LSTAR, (uint64_t) &syscall_entry);
    msr_write(MSR_SFMASK, msr_read(MSR_SFMASK) | (1 << 9));
}