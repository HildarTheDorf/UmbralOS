#include "intel.h"

struct cpuid_result cpuid(uint32_t eax, uint32_t ecx) {
    struct cpuid_result ret;
    __asm("cpuid"
        : "=a"(ret.eax), "=b"(ret.ebx), "=c"(ret.ecx), "=d"(ret.edx)
        : "a"(eax), "c"(ecx));
    return ret;
}

uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm volatile("in %1,%0" : "=a"(value) : "Nd"(port));
    return value;
}

void outb(uint16_t port, uint8_t value) {
    __asm("out %0,%1" : :  "a"(value), "Nd"(port));
}
