#pragma once

/**
 * @brief Divide and round up
 * @param DIVIDEND
 * @param DIVISOR
 * @returns result
 */
#define ROUND_DIV_UP(DIVIDEND, DIVISOR) (((DIVIDEND) + (DIVISOR) - 1) / (DIVISOR))

/**
 * @brief Round up
 * @param VALUE
 * @param PRECISION
 * @returns result
 */
#define ROUND_UP(VALUE, PRECISION) (ROUND_DIV_UP((VALUE), (PRECISION)) * (PRECISION))

/**
 * @brief Round down
 * @param VALUE
 * @param PRECISION
 * @returns result
 */
#define ROUND_DOWN(VALUE, PRECISION) (((VALUE) / (PRECISION)) * (PRECISION))