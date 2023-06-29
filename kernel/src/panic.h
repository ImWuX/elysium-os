#ifndef PANIC_H
#define PANIC_H

#include <arch/amd64/interrupt.h>

/**
 * @brief Log an issue and halt the core
 * 
 * @param location The location where the issue was encountered
 * @param msg A message explaining the issue
*/
[[noreturn]] void panic(char *location, char *msg);

/**
 * @brief Exception handler that panics and halts the core
 *
 * @param frame Interrupt frame
*/
[[noreturn]] void panic_exception(interrupt_frame_t *frame);


#endif