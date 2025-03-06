#include "serial.h"

#include <stdint.h>

#define COM1 0x3f8

static uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm("in %1,%0" : "=a"(value) : "Nd"(port));
    return value;
}

static void outb(uint16_t port, uint8_t value) {
    __asm("out %0,%1" : :  "a"(value), "Nd"(port));
}

void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0F);
}

void serial_putc(char c) {
    while (!(inb(COM1 + 5) & 0x20));

    outb(COM1, c);
}
