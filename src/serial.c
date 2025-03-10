#include "serial.h"

#include <stdint.h>

#define COM1 0x3f8

static uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm volatile("in %1,%0" : "=a"(value) : "Nd"(port));
    return value;
}

static void outb(uint16_t port, uint8_t value) {
    __asm("out %0,%1" : :  "a"(value), "Nd"(port));
}

void serial_init(void) {
    outb(COM1 + 1, 0x00); // Disable Interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB
    outb(COM1 + 0, 0x01); // Baud Rate Divisor (LSB) 
    outb(COM1 + 1, 0x00); // Baud Rate Divisor (MSB)
    outb(COM1 + 3, 0x03); // Disable DLAB, 8 data-bits, 1 stop-bit, 0 parity-bits
    outb(COM1 + 2, 0xC7); // FIFOs enabled and both cleared, trigger interrupt at 14 bytes
    outb(COM1 + 4, 0x0B); // Set DTR, RTS, OUT2(connect to IRQ line)
}

void serial_putc(char c) {
    while (!(inb(COM1 + 5) & 0x20)); // Wait for transmit buffer to be empty

    outb(COM1, c);
}
