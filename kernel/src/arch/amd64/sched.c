#include "sched.h"
#include <arch/types.h>
#include <memory/heap.h>
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

uint8_t g_sched_vector;

void sched_entry(interrupt_frame_t *frame) {
    sched_thread_t *current_thread = sched_get_current_thread();
    arch_cpu_local_t *cpu_local = current_thread->cpu_local;
    sched_thread_t *next_thread = sched_next_thread(current_thread);
    if(!next_thread || current_thread == next_thread) goto eoi;

    FRAME_TO_REGS(frame, current_thread->context.registers);
    __atomic_store_n(&current_thread->lock, 0, __ATOMIC_SEQ_CST);

    REGS_TO_FRAME(next_thread->context.registers, frame);
    next_thread->cpu_local = cpu_local;
    sched_set_current_thread(next_thread);

    eoi:
    lapic_eoi(frame->int_no);
    lapic_timer_oneshot(g_sched_vector, 1'000'000);
}