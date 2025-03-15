#pragma once

#include <stdarg.h>
#include <stddef.h>

[[noreturn]]
void halt(void);
[[noreturn]]
void halt_and_catch_fire(void);

[[gnu::format(printf, 1, 2)]]
void kprint(const char *format, ...);

[[gnu::format(printf, 1, 0)]]
void kprintv(const char *format, va_list va);

void memset(void *s, int c, size_t n);
void memzero(void *s, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);

[[noreturn]]
[[gnu::format(printf, 1, 2)]]
void panic(const char *fmt, ...);
