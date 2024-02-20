#pragma once
#include <stdint.h>

#define X86_64_GDT_CODE_RING0 0x8
#define X86_64_GDT_DATA_RING0 0x10
#define X86_64_GDT_CODE_RING3 0x20
#define X86_64_GDT_DATA_RING3 0x18

/**
 * @brief Loads the GDT
 */
void x86_64_gdt_load();