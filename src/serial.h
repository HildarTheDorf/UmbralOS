#pragma once

#include <stddef.h>

void serial_init(void);
char serial_read(void);
void serial_write(const char *buf, size_t size);
