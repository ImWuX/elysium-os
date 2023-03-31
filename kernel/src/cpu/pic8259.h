#ifndef CPU_PIC8259_H
#define CPU_PIC8259_H

#include <stdint.h>

void pic8259_remap();
void pic8259_disable();
void pic8259_eoi(uint8_t interrupt_vector);

#endif