#ifndef PROC_SCHED_H
#define PROC_SCHED_H

#include <stdint.h>
#include <stdnoreturn.h>

noreturn void sched_handoff();
noreturn extern void sched_enter(uint64_t address);

#endif