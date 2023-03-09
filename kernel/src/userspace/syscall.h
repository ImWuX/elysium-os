#ifndef USERSPACE_SYSCALL_H
#define USERSPACE_SYSCALL_H

#include <stdint.h>
#include <stdbool.h>

bool syscall_available();
void syscall_initialize();

extern void syscall_entry();

#endif