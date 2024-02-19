#pragma once
#include <stddef.h>

/**
 * @brief Fill memory with a value
 * @param dest
 * @param ch value to fill memory with
 * @param count amount of memory to fill
 * @returns destination
 */
void *memset(void *dest, int ch, size_t count);

/**
 * @brief Copy memory
 * @param dest
 * @param src
 * @param count amount of memory to copy
 * @returns destination
 */
void *memcpy(void *dest, const void *src, size_t count);

/**
 * @brief Compare two regions of memory. Results in a negative value if the LHS is greater, a positive value if the RHS is greater, and a zero if they're equal
 * @param lhs left-hand side of the comparison
 * @param rhs light-hand side of the comparison
 * @param count amount of memory to compare
 * @returns result of the comparison
 */
int memcmp(const void *lhs, const void *rhs, size_t count);