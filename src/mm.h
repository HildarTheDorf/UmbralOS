#pragma once

#include <limine.h>

#include <stddef.h>

#define PAGE_SIZE 0x1000

enum memory_flags {
    M_NONE = 0x0,

    M_W = 0x1,
    M_X = 0x2,
    M_U = 0x4,

    M_CACHE_WB = 0x0,
    M_CACHE_UC = 0x8,
    M_CACHE_WC = 0x10,
    M_CACHE_MASK = 0x18,
};

typedef uint32_t phy32_t;
typedef uint64_t phy_t;

void *kalloc(size_t size);
void kfree(void *ptr, size_t size);
void *phy_to_virt(phy_t paddr);
phy_t pmm_alloc_page(void);
phy_t pmm_alloc_pages(size_t num_pages);
void pmm_free_page(phy_t paddr);
void pmm_free_pages(phy_t paddr, size_t num_pages);
void pmm_init(const struct limine_memmap_response *limine_memmap_response, void *phhdm);
void pmm_reclaim(const struct limine_memmap_response *limine_memmap_response, void *stack_origin, size_t stack_size);
#ifdef DEBUG_CHECKS
void pmm_zero(void);
#endif
void vmm_init(const struct limine_memmap_response *limine_memmap_response, const struct limine_executable_address_response *limine_executable_address_response);
void vmm_map(phy_t what, void *where, size_t size, enum memory_flags flags);
void vmm_map_unaligned(phy_t what, void *where, size_t size, enum memory_flags flags);
