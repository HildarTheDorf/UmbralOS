#pragma once

#include <stddef.h>

// Configures the serial port for output only
void serial_early_init(void);

// Configures the serial port for input/output.
// Calls serial_early_init if needed.
void serial_init(void);

void serial_handle_interrupt(void);
char serial_read(void);
void serial_write(const char *buf, size_t size);
