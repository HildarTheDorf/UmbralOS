#include "common.h"

[[noreturn]]
void halt(void) {
    __asm("cli");
    while (true) {
        __asm("hlt");
    }
}

[[noreturn]]
void panic(const char *format, ...) {
    halt();
}
