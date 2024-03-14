#pragma once
#include <stdint.h>

#define PIT_FREQ 1193182

/**
 * @brief Set the reload value
 */
void x86_64_pit_set_reload(uint16_t reload_value);

/**
 * @brief Set the frequency
 */
void x86_64_pit_set_frequency(uint64_t frequency);

/**
 * @brief Retrieve current PIT count
 * @return Current PIT count
 */
uint16_t x86_64_pit_count();