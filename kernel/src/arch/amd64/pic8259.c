#include "pic8259.h"
#include <arch/amd64/io.h>

#define MASTER_CMD 0x20
#define MASTER_DATA 0x21
#define SLAVE_CMD 0xA0
#define SLAVE_DATA 0xA1

void pic8259_remap() {
    io_port_outb(MASTER_CMD, 0x11);
    io_port_outb(SLAVE_CMD, 0x11);
    io_port_outb(MASTER_DATA, 0x20);
    io_port_outb(SLAVE_DATA, 0x28);
    io_port_outb(MASTER_DATA, 0x04);
    io_port_outb(SLAVE_DATA, 0x02);
    io_port_outb(MASTER_DATA, 0x01);
    io_port_outb(SLAVE_DATA, 0x01);
    io_port_outb(MASTER_DATA, 0x0);
    io_port_outb(SLAVE_DATA, 0x0);
}

void pic8259_disable() {
    io_port_outb(MASTER_DATA, 0xFF);
    io_port_outb(SLAVE_DATA, 0xFF);
}

void pic8259_eoi(uint8_t interrupt_vector) {
    if(interrupt_vector >= 40) io_port_outb(SLAVE_CMD, 0x20);
    io_port_outb(MASTER_CMD, 0x20);
}