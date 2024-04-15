#pragma once
#include <stdint.h>
#include <arch/x86_64/sys/tss.h>

#define X86_64_GDT_SELECTOR_CODE64_RING0 (1 << 3)
#define X86_64_GDT_SELECTOR_DATA64_RING0 (2 << 3)
#define X86_64_GDT_SELECTOR_DATA64_RING3 ((3 << 3) | 0b11)
#define X86_64_GDT_SELECTOR_CODE64_RING3 ((4 << 3) | 0b11)

/**
 * @brief Loads the GDT
 */
void x86_64_gdt_load();

/**
 * @brief Loads a TSS
 */
void x86_64_gdt_load_tss(x86_64_tss_t *tss);