#include "gdt.h"
#include "interrupt.h"

#include <limine.h>

LIMINE_REQUESTS_START_MARKER
LIMINE_BASE_REVISION(3)
LIMINE_REQUESTS_END_MARKER

[[noreturn]] static void halt(void) {
    __asm("cli");
    while (true) {
        __asm("hlt");
    }
}

[[noreturn]] void _start(void) {
    load_gdt();         
    load_idt();

    __asm("int $3");

    halt();
}
