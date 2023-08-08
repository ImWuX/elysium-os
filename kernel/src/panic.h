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

#endif