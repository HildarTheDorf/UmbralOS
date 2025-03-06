#include <stdbool.h>

[[noreturn]] void _start() {
    while (true) {
        asm("hlt");
    }
}
