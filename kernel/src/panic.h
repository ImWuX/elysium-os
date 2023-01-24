#ifndef PANIC_H
#define PANIC_H

#include <stdnoreturn.h>

noreturn void panic(char *location, char *msg);

#endif