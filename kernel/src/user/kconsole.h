#ifndef USER_KCONSOLE_H
#define USER_KCONSOLE_H

#include <stdint.h>

void initialize_kconsole(int x, int y, int w, int h, uint64_t bios_memory_map_address);
int putchar(int character);
void keyboard_handler(uint8_t character);

#endif