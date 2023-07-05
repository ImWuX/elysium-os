#include "sched.h"
#include <string.h>
#include <arch/sched.h>
#include <arch/types.h>
#include <arch/vmm.h>
#include <memory/heap.h>
#include <memory/hhdm.h>
#include <arch/amd64/lapic.h>
#include <arch/amd64/gdt.h>

#define A(REG, FROM, TO) TO REG = FROM REG;
#define R(FROM, TO)     \
    A(rax, FROM, TO)    \
    A(rbx, FROM, TO)    \
    A(rcx, FROM, TO)    \
    A(rdx, FROM, TO)    \
    A(rbp, FROM, TO)    \
    A(rsi, FROM, TO)    \
    A(rdi, FROM, TO)    \
    A(r8, FROM, TO)     \
    A(r9, FROM, TO)     \
    A(r10, FROM, TO)    \
    A(r11, FROM, TO)    \
    A(r12, FROM, TO)    \
    A(r13, FROM, TO)    \
    A(r14, FROM, TO)    \
    A(r15, FROM, TO)    \
    A(cs, FROM, TO)     \
    A(ds, FROM, TO)     \
    A(ss, FROM, TO)     \
    A(es, FROM, TO)     \
    A(rip, FROM, TO)    \
    A(rflags, FROM, TO) \
    A(rsp, FROM, TO)

#define FRAME_TO_REGS(FRAME, REGS) R((FRAME)->, (REGS).)
#define REGS_TO_FRAME(REGS, FRAME) R((REGS)., (FRAME)->)

extern vmm_address_space_t g_kernel_address_space;

uint8_t g_sched_vector;

void sched_entry(interrupt_frame_t *frame) {
    sched_thread_t *current_thread = arch_sched_get_current_thread();
    arch_cpu_local_t *cpu_local = current_thread->cpu_local;
    sched_thread_t *next_thread = sched_next_thread(current_thread);
    if(!next_thread || current_thread == next_thread) goto eoi;

    FRAME_TO_REGS(frame, current_thread->context.registers);
    __atomic_store_n(&current_thread->lock, 0, __ATOMIC_SEQ_CST);
    REGS_TO_FRAME(next_thread->context.registers, frame);
    arch_vmm_load_address_space(next_thread->address_space);
    next_thread->cpu_local = cpu_local;
    arch_sched_set_current_thread(next_thread);

    eoi:
    lapic_eoi(frame->int_no);
    lapic_timer_oneshot(g_sched_vector, 1'000'000);
}

sched_thread_t *arch_sched_get_current_thread() {
    sched_thread_t *thread = 0;
    asm volatile("mov %%gs:0, %0" : "=r" (thread));
    return thread;
}

void arch_sched_set_current_thread(sched_thread_t *thread) {
    msr_write(MSR_GS_BASE, (uint64_t) thread);
}

void arch_sched_init_kernel_thread(sched_thread_t *thread, void *entry) {
    memset(thread, 0, sizeof(sched_thread_t));
    thread->this = thread;
    thread->id = 0;
    thread->context.registers.cs = GDT_CODE_RING0;
    thread->context.registers.ss = GDT_DATA_RING0;
    thread->context.registers.rflags = (1 << 9) | (1 << 1);
    thread->context.registers.rsp = HHDM(pmm_page_alloc(PMM_PAGE_USAGE_WIRED)->paddr + ARCH_PAGE_SIZE);
    thread->context.registers.rip = (uint64_t) entry;
    thread->address_space = &g_kernel_address_space;
}