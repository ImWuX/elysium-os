#ifndef PANIC_H
#define PANIC_H

#include <stdnoreturn.h>
#include <cpu/exceptions.h>

noreturn void panic(char *location, char *msg);
noreturn void panic_exception(char *msg, exception_frame_t regs);

#endif