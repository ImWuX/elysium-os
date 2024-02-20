#pragma once

typedef enum {
    X86_64_INIT_STAGE_ONE,
    X86_64_INIT_STAGE_DONE
} x86_64_init_stage_t;

/**
 * @brief Retrieves the current initialization stage
 */
x86_64_init_stage_t x86_64_init_stage();