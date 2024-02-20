#pragma once

typedef enum {
    X86_64_INIT_STAGE_ENTRY,
    X86_64_INIT_STAGE_PHYS_MEMORY,
    X86_64_INIT_STAGE_INTERRUPTS,
    X86_64_INIT_STAGE_MEMORY,
    X86_64_INIT_STAGE_FINAL
} x86_64_init_stage_t;

/**
 * @brief Retrieves the current initialization stage
 */
x86_64_init_stage_t x86_64_init_stage();