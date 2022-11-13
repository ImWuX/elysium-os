#ifndef CPU_ISR_H
#define CPU_ISR_H

#include <stdint.h>

typedef struct {
    uint64_t ds;
    uint64_t rdi, rsi, rbp, rsp, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, eflags, useresp, ss;
} irq_cpu_register_t;

typedef void (*interrupt_handler_t)(irq_cpu_register_t);

void initialize_irqs();
void register_interrupt_handler(uint8_t id, interrupt_handler_t interrupt_handler);
void irq_handler(irq_cpu_register_t regs);

extern void irq_32();
extern void irq_33();
extern void irq_34();
extern void irq_35();
extern void irq_36();
extern void irq_37();
extern void irq_38();
extern void irq_39();
extern void irq_40();
extern void irq_41();
extern void irq_42();
extern void irq_43();
extern void irq_44();
extern void irq_45();
extern void irq_46();
extern void irq_47();

#endif