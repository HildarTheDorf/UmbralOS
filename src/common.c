#include "common.h"
#include "serial.h"

[[noreturn]]
void halt(void) {
    __asm("cli");
    while (true) {
        __asm("hlt");
    }
}

__attribute((format(printf, 1, 2)))
void kprint(const char *format, ...) {
    va_list va;
    va_start(va);
    kprintv(format, va);
    va_end(va);
}

__attribute((format(printf, 1, 0)))
void kprintv(const char *format, va_list va) {
    for (const char *p = format; *p; ++p) {
        serial_putc(*p);
    }
}

[[noreturn]]
__attribute((format(printf, 1, 2)))
void panic(const char *format, ...) {
    kprint("PANIC: ");

    va_list va;
    va_start(va);
    kprint(format, va);
    va_end(va);

    kprint("\n");

    halt();
}
