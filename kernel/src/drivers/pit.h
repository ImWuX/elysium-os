#ifndef DRIVERS_PIT_H
#define DRIVERS_PIT_H

#include <stdint.h>

void pit_initialize();
uint64_t pit_time_ms();
uint64_t pit_time_s();

#endif