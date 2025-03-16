#pragma once

#include <stddef.h>

void serial_init(void);
void serial_write(const char *buf, size_t size);
