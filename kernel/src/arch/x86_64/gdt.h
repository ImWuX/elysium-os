#pragma once
#include <stdint.h>

#define GDT_CODE_RING0 0x8
#define GDT_DATA_RING0 0x10
#define GDT_CODE_RING3 0x20
#define GDT_DATA_RING3 0x18

/**
 * @brief Loads the GDT.
 */
void gdt_load();