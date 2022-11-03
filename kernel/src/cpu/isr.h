#ifndef CPU_ISR_H
#define CPU_ISR_H

#include <stdint.h>

#define IRQ_OFFSET 32

typedef struct {
    uint64_t ds;
    uint64_t rdi, rsi, rbp, rsp, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, eflags, useresp, ss;
} cpu_register_t;

typedef void (*interrupt_handler_t)(cpu_register_t);

void initialize_isrs();
void initialize_irqs();
void register_interrupt_handler(uint8_t id, interrupt_handler_t interrupt_handler);

void isr_handler(cpu_register_t regs);
void irq_handler(cpu_register_t regs);

extern void isr_0();
extern void isr_1();
extern void isr_2();
extern void isr_3();
extern void isr_4();
extern void isr_5();
extern void isr_6();
extern void isr_7();
extern void isr_8();
extern void isr_9();
extern void isr_10();
extern void isr_11();
extern void isr_12();
extern void isr_13();
extern void isr_14();
extern void isr_15();
extern void isr_16();
extern void isr_17();
extern void isr_18();
extern void isr_19();
extern void isr_20();
extern void isr_21();
extern void isr_22();
extern void isr_23();
extern void isr_24();
extern void isr_25();
extern void isr_26();
extern void isr_27();
extern void isr_28();
extern void isr_29();
extern void isr_30();
extern void isr_31();

extern void irq_0();
extern void irq_1();
extern void irq_2();
extern void irq_3();
extern void irq_4();
extern void irq_5();
extern void irq_6();
extern void irq_7();
extern void irq_8();
extern void irq_9();
extern void irq_10();
extern void irq_11();
extern void irq_12();
extern void irq_13();
extern void irq_14();
extern void irq_15();

#endif