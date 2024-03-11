#pragma once

/**
 * @brief Divide and round up
 * @param DIVIDEND
 * @param DIVISOR
 * @returns result
 */
#define MATH_DIV_CEIL(DIVIDEND, DIVISOR) (((DIVIDEND) + (DIVISOR) - 1) / (DIVISOR))

/**
 * @brief Round up
 * @param VALUE
 * @param PRECISION
 * @returns result
 */
#define MATH_CEIL(VALUE, PRECISION) (MATH_DIV_CEIL((VALUE), (PRECISION)) * (PRECISION))

/**
 * @brief Round down
 * @param VALUE
 * @param PRECISION
 * @returns result
 */
#define MATH_FLOOR(VALUE, PRECISION) (((VALUE) / (PRECISION)) * (PRECISION))

/**
 * @brief Find the minimum number
 */
int math_min(int a, int b);

/**
 * @brief Find the maximum number
 */
int math_max(int a, int b);