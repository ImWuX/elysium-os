#ifndef CPU_TIMER_H
#define CPU_TIMER_H

#define TIMER_FREQUENCY 1000

#include <stdint.h>

void initialize_timer();
uint64_t get_time_ms();
uint64_t get_time_s();

#endif