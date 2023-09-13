#pragma once
#include <stdint.h>

#ifdef __ARCH_AMD64

#define ARCH_PAGE_SIZE 0x1000

#define ARCH_USERSPACE_START 0x1000
#define ARCH_HHDM_START 0xFFFF'8000'0000'0000
#define ARCH_HHDM_END 0xFFFF'8400'0000'0000
#define ARCH_KHEAP_START 0xFFFF'8400'0000'0000
#define ARCH_KHEAP_END 0xFFFF'8500'0000'0000

typedef struct {
    uint64_t lapic_timer_freq;
} arch_cpu_local_t;

typedef struct {
    uintptr_t cr3;
} arch_vmm_address_space_t;

typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r12;
    uint64_t r13;
    uint64_t r11;
    uint64_t r14;
    uint64_t r15;
    uint64_t cs;
    uint64_t ds;
    uint64_t ss;
    uint64_t es;
    uint64_t rip;
    uint64_t rflags;
    uint64_t rsp;
} arch_cpu_registers_t;

typedef struct {
    arch_cpu_registers_t registers;
} arch_cpu_context_t;

#else
#error Invalid arch
#endif