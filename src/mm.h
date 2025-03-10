#pragma once

#include <limine.h>

#define PAGE_SIZE 0x1000

typedef uint64_t phy_t;

void *phy_to_virt(phy_t paddr);
phy_t pmm_alloc_page(void);
void pmm_free_page(phy_t);
void pmm_init(const struct limine_memmap_response *limine_memmap_response, void *phhdm);
void vmm_init(const struct limine_memmap_response *limine_memmap_response, const struct limine_kernel_address_response *limine_kernel_address_response);
