#pragma once
#include <stdint.h>
#include <stdarg.h>

typedef void (* format_out_t)(char ch);

/**
 * @brief Prints a formatted string. Tries to adhere to C spec
 * @param out Function to pipe result to
 * @returns char count written
 */
int format(format_out_t out, const char *format, va_list list);