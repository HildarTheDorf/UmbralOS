#include "common.h"
#include "serial.h"

#include <stdbool.h>
#include <stdint.h>

#define NUM_BUF_LEN 20

static void print_char(char c) {
    serial_putc(c);
}

static void print_string(const char *str) {
    for (const char *p = str; *p; ++p) {
        print_char(*p);
    }
}

static void print_unsigned(uint64_t value, uint8_t base) {
    char buf[NUM_BUF_LEN + 1];
    buf[NUM_BUF_LEN - 1] = '0';
    buf[NUM_BUF_LEN] = '\0';

    int start = NUM_BUF_LEN - 1;
    for (int i = 0; i < 16 && value; ++i) {
        const char ch_idx = value % base;
        uint8_t buf_idx = NUM_BUF_LEN - 1 - i;
        if (ch_idx < 10) {
            buf[buf_idx] = ch_idx + '0';
        } else {
            buf[buf_idx] = ch_idx - 10 + 'A';
        }
        if (ch_idx != 0) {
            start = buf_idx;
        }
        value /= base;
    }
    print_string(buf + start);
}

static void print_escape(const char **p, va_list va, bool is_long) {
    char c = **p;
    switch (c) {
    case 'c':
        print_char(va_arg(va, int));
        break;
    case 'l':
        ++*p;
        print_escape(p, va, true);
        break;
    case 'p':
        const void *p_value = va_arg(va, const void *);
        print_unsigned((uintptr_t)p_value, 16);
        break;
    case 's':
        print_string(va_arg(va, const char *));
        break;
    case 'u':
        uint64_t u_value = is_long ? va_arg(va, uint64_t) : va_arg(va, uint32_t);
        print_unsigned(u_value, 10);
        break;
    case 'x':
        uint64_t x_value = is_long ? va_arg(va, uint64_t) : va_arg(va, uint32_t);
        print_unsigned(x_value, 16);
        break;
    default:
        panic("Unknown character %c in print frormat", c);
    }
}

[[noreturn]]
void halt(void) {
    while (true) {
        __asm("hlt");
    }
}

[[noreturn]]
void halt_and_catch_fire(void) {
    __asm("cli");
    halt();
}

[[gnu::format(printf, 1, 2)]]
void kprint(const char *format, ...) {
    va_list va;
    va_start(va);
    kprintv(format, va);
    va_end(va);
}

[[gnu::format(printf, 1, 0)]]
void kprintv(const char *format, va_list va) {
    for (const char *p = format; *p; ++p) {
        if (*p == '%') {
            ++p;
            print_escape(&p, va, false);
        } else {
            print_char(*p);
        }
    }
}

void memset(void *s, int c, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        ((char *)s)[i] = c;
    }
}

void memzero(void *s, size_t n) {
    memset(s, 0, n);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (s1[i] != s2[i]) return s1[i] - s2[i];
        if (s1[i] == '\0') return 0;
    }
    return 0;
}

[[noreturn, gnu::format(printf, 1, 2)]]
void panic(const char *format, ...) {
    kprint("PANIC: ");

    va_list va;
    va_start(va);
    kprintv(format, va);
    va_end(va);

    kprint("\n");
    halt_and_catch_fire();
}
