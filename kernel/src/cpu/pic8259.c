#include "pic8259.h"
#include <drivers/ports.h>

#define MASTER_CMD 0x20
#define MASTER_DATA 0x21
#define SLAVE_CMD 0xA0
#define SLAVE_DATA 0xA1

void pic8259_remap() {
    ports_outb(MASTER_CMD, 0x11);
    ports_outb(SLAVE_CMD, 0x11);
    ports_outb(MASTER_DATA, 0x20);
    ports_outb(SLAVE_DATA, 0x28);
    ports_outb(MASTER_DATA, 0x04);
    ports_outb(SLAVE_DATA, 0x02);
    ports_outb(MASTER_DATA, 0x01);
    ports_outb(SLAVE_DATA, 0x01);
    ports_outb(MASTER_DATA, 0x0);
    ports_outb(SLAVE_DATA, 0x0);
}

void pic8259_disable() {
    ports_outb(MASTER_DATA, 0xFF);
    ports_outb(SLAVE_DATA, 0xFF);
}

void pic8259_eoi(uint8_t interrupt_vector) {
    if(interrupt_vector >= 40) ports_outb(SLAVE_CMD, 0x20);
    ports_outb(MASTER_CMD, 0x20);
}