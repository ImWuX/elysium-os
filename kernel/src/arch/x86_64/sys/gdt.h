#pragma once
#include <stdint.h>
#include <arch/x86_64/sys/tss.h>

#define X86_64_GDT_CODE_RING0 0x8
#define X86_64_GDT_DATA_RING0 0x10
#define X86_64_GDT_CODE_RING3 0x20
#define X86_64_GDT_DATA_RING3 0x18
#define X86_64_GDT_TSS 0x28

/**
 * @brief Loads the GDT
 */
void x86_64_gdt_load();

/**
 * @brief Loads a TSS
 */
void x86_64_gdt_load_tss(x86_64_tss_t *tss);