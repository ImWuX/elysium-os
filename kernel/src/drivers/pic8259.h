#ifndef HAL_PIC8259_H
#define HAL_PIC8259_H

#include <stdbool.h>

void pic8259_remap();
void pic8259_disable();
void pic8259_eoi(bool slave);

#endif