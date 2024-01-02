#include "pic8259.h"
#include <arch/x86_64/port.h>

#define MASTER_CMD 0x20
#define MASTER_DATA 0x21
#define SLAVE_CMD 0xA0
#define SLAVE_DATA 0xA1

void pic8259_remap() {
    port_outb(MASTER_CMD, 0x11);
    port_outb(SLAVE_CMD, 0x11);
    port_outb(MASTER_DATA, 0x20);
    port_outb(SLAVE_DATA, 0x28);
    port_outb(MASTER_DATA, 0x04);
    port_outb(SLAVE_DATA, 0x02);
    port_outb(MASTER_DATA, 0x01);
    port_outb(SLAVE_DATA, 0x01);
    port_outb(MASTER_DATA, 0x0);
    port_outb(SLAVE_DATA, 0x0);
}

void pic8259_disable() {
    port_outb(MASTER_DATA, 0xFF);
    port_outb(SLAVE_DATA, 0xFF);
}

void pic8259_eoi(uint8_t interrupt_vector) {
    if(interrupt_vector >= 40) port_outb(SLAVE_CMD, 0x20);
    port_outb(MASTER_CMD, 0x20);
}