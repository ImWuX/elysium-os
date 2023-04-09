#ifndef CPU_INTERRUPT_H
#define CPU_INTERRUPT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t int_no;
    uint64_t err_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} interrupt_frame_t;

typedef struct {
    uint16_t low_offset;
    uint16_t segment_selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t middle_offset;
    uint32_t high_offset;
    uint32_t rsv0;
} __attribute__((packed)) interrupt_idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) interrupt_idt_descriptor_t;

typedef enum {
    INTERRUPT_PRIORITY_EXCEPTION = 0,
    INTERRUPT_PRIORITY_DISK = 10,
    INTERRUPT_PRIORITY_HID = 13,
    INTERRUPT_PRIORITY_TIMER = 14,
    INTERRUPT_PRIORITY_KERNHIGH = 15
} interrupt_priority_t;

typedef void (* interrupt_handler_t)(interrupt_frame_t *frame);
typedef void (* interrupt_irq_eoi_t)(uint8_t);

typedef struct {
    bool free;
    interrupt_handler_t handler;
    interrupt_priority_t priority;
} interrupt_entry_t;

extern interrupt_irq_eoi_t g_interrupt_irq_eoi;

void interrupt_initialize();
void interrupt_handler(interrupt_frame_t *frame);
void interrupt_set(uint8_t vector, interrupt_priority_t priority, interrupt_handler_t handler);
int interrupt_request(interrupt_priority_t priority, interrupt_handler_t handler);
void interrupt_irq_eoi(uint8_t vector);

#endif