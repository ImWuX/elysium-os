#ifndef DRIVERS_PIT_H
#define DRIVERS_PIT_H

#include <stdint.h>

typedef struct pit_interval_struct {
    uint64_t offset;
    uint64_t interval;
    void (* cb)();
    struct pit_interval_struct *next;
} pit_interval_t;

void pit_initialize();
void pit_interval(int interval, void (* cb)());
uint64_t pit_time_ms();
uint64_t pit_time_s();

#endif