#include "security.h"

#include "intel.h"

static bool HAS_SMAP;

void enforce_smap(void) {
    if (HAS_SMAP) __asm("clac");
}

void unenforce_smap(void) {
    if (HAS_SMAP) __asm("stac");
}

void security_init(void) {
    const struct cpuid_result cpuid70 = cpuid(7,0);

    HAS_SMAP = cpuid70.ebx & CPUID_7_0_EBX_SMAP;
    const bool has_smep = cpuid70.ebx & CPUID_7_0_EBX_SMEP;
    const bool has_umip = cpuid70.ecx & CPUID_7_0_ECX_UMIP;

    uint64_t cr4;
    __asm volatile("mov %%cr4,%0" : "=r"(cr4));
    if (HAS_SMAP) cr4 |= CR4_SMAP;
    if (has_smep) cr4 |= CR4_SMEP;
    if (has_umip) cr4 |= CR4_UMIP;
    __asm("mov %0,%%cr4" : : "r"(cr4));

    enforce_smap();
}
