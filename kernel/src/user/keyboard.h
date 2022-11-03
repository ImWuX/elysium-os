#ifndef USER_KEYBOARD_H
#define USER_KEYBOARD_H

#include <stdint.h>

typedef void (* keyboard_handler_t)(uint8_t);

void initialize_keyboard();
void set_keyboard_handler(keyboard_handler_t keyboard_handler);

#endif