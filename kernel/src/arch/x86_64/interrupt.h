#pragma once
#include <stdint.h>

typedef struct {
    uint64_t ds;
    uint64_t es;
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
} x86_64_interrupt_frame_t;

typedef enum {
    INTERRUPT_PRIORITY_EXCEPTION = 0x0,
    INTERRUPT_PRIORITY_NORMAL = 0x5,
    INTERRUPT_PRIORITY_IPC = 0xE,
    INTERRUPT_PRIORITY_CRITICAL = 0xF
} x86_64_interrupt_priority_t;

typedef void (* x86_64_interrupt_handler_t)(x86_64_interrupt_frame_t *frame);
typedef void (* x86_64_interrupt_irq_eoi_t)(uint8_t);

extern x86_64_interrupt_irq_eoi_t g_x86_64_interrupt_irq_eoi;

/**
 * @brief Initialize the IDT and interrupt management.
 */
void x86_64_interrupt_init();

/**
 * @brief Loads the IDT.
 */
void x86_64_interrupt_load_idt();

/**
 * @brief Set a handler onto an interrupt vector.
 * @warning Will carelessly override existing handlers.
 * @param vector
 * @param priority
 * @param handler
 */
void x86_64_interrupt_set(uint8_t vector, x86_64_interrupt_priority_t priority, x86_64_interrupt_handler_t handler);

/**
 * @brief Request a free interrupt vector.
 * @param priority
 * @param handler
 * @return chosen interrupt vector, -1 on error
 */
int x86_64_interrupt_request(x86_64_interrupt_priority_t priority, x86_64_interrupt_handler_t handler);
