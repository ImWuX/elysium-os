#include "ports.h"

uint8_t ports_inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

void ports_outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a" (value), "Nd" (port));
}

uint32_t ports_inl(uint16_t port) {
    uint32_t result;
    asm volatile("inl %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

void ports_outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a" (value), "Nd" (port));
}