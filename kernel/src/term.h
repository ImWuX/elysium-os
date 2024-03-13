#pragma once
#include <graphics/draw.h>

/**
 * @brief Initialize the terminal
 */
void term_init(draw_context_t *context);

/**
 * @brief Close the terminal
 */
void term_close();

/**
 * @brief Hook for handling keystrokes
 */
void term_kb_handler(uint8_t ch);