#ifndef ARCH_AMD64_EXCEPTION_H
#define ARCH_AMD64_EXCEPTION_H

#include <arch/amd64/interrupt.h>

[[noreturn]] void exception_unhandled(interrupt_frame_t *frame);

#endif