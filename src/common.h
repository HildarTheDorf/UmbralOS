#pragma once

#include <stdarg.h>
#include <stddef.h>

[[noreturn]]
void halt(void);

__attribute((format(printf, 1, 2)))
void kprint(const char *format, ...);

__attribute((format(printf, 1, 0)))
void kprintv(const char *format, va_list va);

void memset(void *s, int c, size_t n);
void memzero(void *s, size_t n);

[[noreturn]]
__attribute((format(printf, 1, 2)))
void panic(const char *fmt, ...);
