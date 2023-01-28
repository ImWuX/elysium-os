#ifndef KCON_H
#define KCON_H

#include <stdint.h>

void kcon_initialize(int width, int height, int x, int y);
void kcon_print_prefix();
void kcon_keyboard_handler(uint8_t character);

int putchar(int c);

#endif