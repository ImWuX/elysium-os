#pragma once
#include <stddef.h>

/**
 * @brief Retrieve the length of a string. Not including the null terminator
 *
 * @param str String to compute the length of
 * @return Length of the string 
 */
size_t strlen(const char *str);

/**
 * @brief Compare two strings. Results in a negative value if the LHS string is greater, a positive number if the RHS is greater, and a zero if they're equal
 *
 * @param lhs Left-hand side string of the comparison
 * @param rhs Right-hand side string of the comparison
 * @return Result of the comparison
 */
int strcmp(const char *lhs, const char *rhs);

/**
 * @brief Copy a string. Including the null terminator
 *
 * @param dest Destination for the string
 * @param src Source string
 * @return The destination
 */
char *strcpy(char *dest, const char *src);

/**
 * @brief Copy a string. If a null terminator is reached, the string will be padded with NULLS
 *
 * @param dest Destination buffer
 * @param src Source buffer
 * @param n Size
 * @return The destination
 */
char *strncpy(char *dest, const char *src, size_t n);

/**
 * @brief Fill memory with a value
 *
 * @param dest Destination
 * @param ch Value to fill memory with
 * @param count Amount of memory to fill
 * @return The destination
 */
void *memset(void *dest, int ch, size_t count);

/**
 * @brief Copy memory
 *
 * @param dest Destination for the memory
 * @param src Source of memory
 * @param count Amount of memory to copy
 * @return The destination
 */
void *memcpy(void *dest, const void *src, size_t count);

/**
 * @brief Compare two regions of memory. Results in a negative value if the LHS is greater, a positive value if the RHS is greater, and a zero if they're equal
 *
 * @param lhs Left-hand side of the comparison
 * @param rhs Right-hand side of the comparison
 * @param count Amount of memory to compare
 * @return Result of the comparison
 */
int memcmp(const void *lhs, const void *rhs, size_t count);