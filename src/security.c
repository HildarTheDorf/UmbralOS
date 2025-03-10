#include "security.h"

#include <stdint.h>

static bool HAS_SMAP;

struct cpuid_result {
    uint64_t eax, ebx, ecx, edx;
};

static struct cpuid_result cpuid(uint32_t eax, uint32_t ecx) {
    struct cpuid_result ret;
    __asm("cpuid"
        : "=a"(ret.eax), "=b"(ret.ebx), "=c"(ret.ecx), "=d"(ret.edx)
        : "a"(eax), "c"(ecx));
    return ret;
}

void enforce_smap(void) {
    if (HAS_SMAP) __asm("clac");
}

void unenforce_smap(void) {
    if (HAS_SMAP) __asm("stac");
}

void security_init(void) {
    const struct cpuid_result cpuid70 = cpuid(7,0);

    HAS_SMAP = cpuid70.ebx & (1 << 20);
    const bool has_smep = cpuid70.ebx & (1 << 7);
    const bool has_umip = cpuid70.ecx & (1 << 2);

    uint64_t cr4;
    __asm volatile("mov %%cr4,%0" : "=r"(cr4));
    if (HAS_SMAP) cr4 |= (1 << 21);
    if (has_smep) cr4 |= (1 << 20);
    if (has_umip) cr4 |= (1 << 11);
    __asm("mov %0,%%cr4" : : "r"(cr4));

    enforce_smap();
}
