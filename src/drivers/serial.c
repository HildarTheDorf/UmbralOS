#include "serial.h"

#include "common.h"
#include "drivers/pic/ioapic.h"
#include "intel.h"

#define COM1 0x3f8

static bool EARLY_INIT_COMPLETE = false;

static void serial_putc(char c) {
    while (!(inb(COM1 + 5) & 0x20)); // Wait for transmit buffer to be empty

    outb(COM1, c);
}

void serial_early_init(void) {
    outb(COM1 + 1, 0x00); // Disable Interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB
    outb(COM1 + 0, 0x01); // Baud Rate Divisor (LSB) 
    outb(COM1 + 1, 0x00); // Baud Rate Divisor (MSB)
    outb(COM1 + 3, 0x03); // Disable DLAB, 8 data-bits, 1 stop-bit, 0 parity-bits
    outb(COM1 + 2, 0xC7); // FIFOs enabled and both cleared, trigger interrupt at 14 bytes
    outb(COM1 + 4, 0x0B); // Set DTR, RTS, OUT2(connect to IRQ line)
    outb(COM1 + 1, 0x01); // Enable Data Recv interrupt

    EARLY_INIT_COMPLETE = true;
}

void serial_init(void) {
    if (!EARLY_INIT_COMPLETE) {
        serial_early_init();
    }
    ioapic_enable_isa_interrupt(IRQ_COM1);
}

void serial_handle_interrupt(void) {
    const char c = serial_read();
    if (c == '\r') {
        kprint("\n");
    } else {
        kprint("%c", c);
    }
}

char serial_read(void) {
    return inb(COM1);
}

void serial_write(const char *buf, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (buf[i] == '\n') serial_putc('\r');
        serial_putc(buf[i]);
    }
}

