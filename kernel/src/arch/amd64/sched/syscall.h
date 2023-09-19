#pragma once
#include <stdint.h>

typedef int64_t (* syscall_handler_t)(uint64_t arg1);

/**
 * @brief Initializes CPU for syscalls
 */
void syscall_init();