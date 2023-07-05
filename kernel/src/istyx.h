#ifndef ISTYX_H
#define ISTYX_H

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
 * @brief A simple method for the kernel to provide keyboard input to styx
 * 
 * @param ch Character
 */
void istyx_simple_input(uint8_t ch);

#endif