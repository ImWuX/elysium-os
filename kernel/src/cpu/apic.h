#ifndef CPU_APIC_H
#define CPU_APIC_H

#include <stdint.h>

void apic_initialize();
void apic_eoi(uint8_t interrupt_vector);
uint8_t apic_id();

#endif