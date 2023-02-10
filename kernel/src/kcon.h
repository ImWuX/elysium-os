#ifndef KCON_H
#define KCON_H

#include <stdint.h>
#include <graphics/draw.h>

void kcon_initialize(draw_context_t *ctx);
void kcon_keyboard_handler(uint8_t character);

int putchar(int c);

#endif