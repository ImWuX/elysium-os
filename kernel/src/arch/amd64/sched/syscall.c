#include "syscall.h"
#include <lib/kprint.h>
#include <arch/amd64/msr.h>
#include <arch/amd64/gdt.h>

#define MAX_HANDLERS 1
#define MSR_EFER_SCE 1

static syscall_handler_t g_handlers[MAX_HANDLERS];

extern void syscall_entry();

int64_t syscall_call(uint64_t num, uint64_t arg1) {
    if(num >= MAX_HANDLERS) return -1;
    return g_handlers[num](arg1);
}

static int64_t syscall_putchar(uint64_t arg1) {
    putchar((int) arg1);
    return 0;
}

void syscall_init() {
    msr_write(MSR_EFER, msr_read(MSR_EFER) | MSR_EFER_SCE);
    msr_write(MSR_STAR, ((uint64_t) GDT_CODE_RING0 << 32) | ((uint64_t) (GDT_CODE_RING3 - 16) << 48));
    msr_write(MSR_LSTAR, (uint64_t) &syscall_entry);
    msr_write(MSR_SFMASK, msr_read(MSR_SFMASK) | (1 << 9));

    g_handlers[0] = &syscall_putchar;
}