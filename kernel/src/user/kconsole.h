#ifndef USER_KCONSOLE_H
#define USER_KCONSOLE_H

#include <stdint.h>

void initialize_kconsole(int x, int y, int w, int h);
int putchar(int character);
void keyboard_handler(uint8_t character);

#endif