#pragma once

#include <stdarg.h>

[[noreturn]] void halt(void);

__attribute((format(printf, 1, 2)))
void kprint(const char *format, ...);

__attribute((format(printf, 1, 0)))
void kprintv(const char *format, va_list va);

__attribute((format(printf, 1, 2)))
[[noreturn]] void panic(const char *fmt, ...);
