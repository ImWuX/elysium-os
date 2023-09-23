#pragma once
#include <stdint.h>
#include <stddef.h>
#include <graphics/draw.h>

/**
 * @brief Early init meant for setting up styx for early logs from the kernel
 *
 * @param draw_context Drawing context
 */
void istyx_early_initialize(draw_context_t *draw_context);

/**
 * @brief Thread init meant for setting up all capabilities of styx
 */
void istyx_thread_init();

/**
 * @brief A simple method for the kernel to provide keyboard input to istyx
 *
 * @param ch Character
 */
void istyx_simple_input_kb(uint8_t ch);

/**
 * @brief A simple method for the kernel to provide mouse input to istyx
 * 
 * @param rel_x Relative X
 * @param rel_y Relative Y
 * @param buttons Mouse buttons
 */
void istyx_simple_input_mouse(int16_t rel_x, int16_t rel_y, bool buttons[3]);