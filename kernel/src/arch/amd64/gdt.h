#ifndef ARCH_AMD64_GDT_H
#define ARCH_AMD64_GDT_H

#include <stdint.h>
#include <arch/amd64/tss.h>

#define GDT_CODE_RING0 0x8
#define GDT_DATA_RING0 0x10
#define GDT_CODE_RING3 0x20
#define GDT_DATA_RING3 0x18

/**
 * @brief Loads the GDT
 */
void gdt_load();

/**
 * @brief Loads a TSS
 *
 * @param tss Task state segment
 */
void gdt_load_tss(tss_t *tss);

#endif