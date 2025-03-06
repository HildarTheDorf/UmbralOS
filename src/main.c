#include "common.h"
#include "gdt.h"
#include "interrupt.h"
#include "serial.h"

#include <limine.h>

LIMINE_REQUESTS_START_MARKER
LIMINE_BASE_REVISION(3)
LIMINE_REQUESTS_END_MARKER

[[noreturn]] void _start(void) {
    serial_init();

    load_gdt();         
    load_idt();

    __asm("int $3");

    halt();
}
