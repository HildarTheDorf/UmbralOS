#include "intel.h"

struct cpuid_result cpuid(uint32_t eax, uint32_t ecx) {
    struct cpuid_result ret;
    __asm volatile("cpuid"
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

uint64_t rdmsr(uint32_t msr) {
    uint32_t valhigh, vallow;
    __asm volatile("rdmsr" : "=d"(valhigh), "=a"(vallow) : "c"(msr));
    return ((uint64_t)valhigh << 32) | ((uint64_t)vallow);
}

void wrmsr(uint32_t msr, uint64_t value) {
    const uint32_t valhigh = value >> 32;
    const uint32_t vallow = value & 0xFFFF'FFFF;
    __asm("wrmsr" : : "d"(valhigh), "a"(vallow), "c"(msr));
}
