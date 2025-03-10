#include "mm.h"
#include "common.h"
#include "intel.h"

#include <elf.h>

#include <limine.h>
#include <limits.h>
#include <stdint.h>

enum memory_flags {
    M_W = 0x1,
    M_X = 0x2,
    M_U = 0x4
};

static struct {
    size_t num_pages;
    uint8_t *bitmap;
} PMM_DATA;

static void *pHHDM;
static struct page_directory_entry_subdirectory *PML4;

static uint64_t round_down(uint64_t value, uint64_t multiple) {
    if (value % multiple) {
        value = value - (value % multiple);
    }
    return value;
}

static uint64_t round_up(uint64_t value, uint64_t multiple) {
    if (value % multiple) {
        value = round_down(value, multiple) + multiple;
    }
    return value;
}

static void addr_to_bitmap_location(phy_t page, size_t *bitmap_index, uint8_t *bitmap_mask) {
    if (page % PAGE_SIZE) panic("addr_to_bitmap_location: addr 0x%lx not aligned\n", page);
    const size_t page_idx = page / PAGE_SIZE;

    if (page > PMM_DATA.num_pages) panic("page_to_bitmap_location: page out of range\n");

    const uint8_t bit_index = page_idx % CHAR_BIT;
    *bitmap_index = page / CHAR_BIT;
    *bitmap_mask = 1 << bit_index;
}

static void pmm_mark_page(phy_t page, bool usable) {
    size_t bitmap_index;
    uint8_t bitmap_mask;
    addr_to_bitmap_location(page, &bitmap_index, &bitmap_mask);
    if (usable) {
        PMM_DATA.bitmap[bitmap_index] |= bitmap_mask;
    } else {
        PMM_DATA.bitmap[bitmap_index] &= ~bitmap_mask;
    }
}


static bool pmm_query_page(phy_t page) {
    size_t bitmap_index;
    uint8_t bitmap_mask;
    addr_to_bitmap_location(page, &bitmap_index, &bitmap_mask);

    return !!(PMM_DATA.bitmap[bitmap_index] & bitmap_mask);
}

static void pmm_mark_range(phy_t base, size_t len, bool usable) {
    if (base % PAGE_SIZE) panic("pmm_mark_range: base not aligned\n");
    if (len % PAGE_SIZE) panic("pmm_mark_range: len not aligned\n");

    for (size_t i = 0; i < len; i += PAGE_SIZE) {
        pmm_mark_page(base + i, usable);
    }
}

void *phy_to_virt(phy_t paddr) {
    return (void *)((uintptr_t)pHHDM + paddr);
}

phy_t pmm_alloc_page(void) {
    for (phy_t i = 0; i < PMM_DATA.num_pages; ++i) {
        const phy_t addr = i * PAGE_SIZE;
        if (pmm_query_page(addr)) {
            pmm_mark_page(addr, false);
            return addr;
        }
    }

    panic("pmm_alloc_page: No more pages");
}

void pmm_free_page(phy_t addr) {
    pmm_mark_page(addr, true);
}

void pmm_init(const struct limine_memmap_response *limine_memmap_response, void *phhdm) {
    pHHDM = phhdm;

    uintptr_t highest_addr = 0;
    for (uint64_t i = 0; i < limine_memmap_response->entry_count; ++i) {
        const struct limine_memmap_entry *limine_memmap_entry = limine_memmap_response->entries[i];
        uintptr_t entry_end = limine_memmap_entry->base + limine_memmap_entry->length;
        if (entry_end > highest_addr) {
            highest_addr = entry_end;
        }
    }

    PMM_DATA.num_pages = round_up(highest_addr, PAGE_SIZE) / PAGE_SIZE;
    const uint64_t bitmap_size = round_up(PMM_DATA.num_pages, CHAR_BIT) / CHAR_BIT;

    phy_t bitmap_phy = 0;
    for (uint64_t i = 0; i < limine_memmap_response->entry_count; ++i) {
        const struct limine_memmap_entry *limine_memmap_entry = limine_memmap_response->entries[i];
        if (limine_memmap_entry->type == LIMINE_MEMMAP_USABLE && limine_memmap_entry->length > bitmap_size) {
            bitmap_phy = limine_memmap_entry->base;
            break;
        }
    }

    if (!bitmap_phy) {
        panic("Unable to locate usable memory region suitable for PMM bitmap. Needed 0x%lx bytes\n", bitmap_size);
    }

    PMM_DATA.bitmap = phy_to_virt(bitmap_phy);
    for (uint64_t i = 0; i < limine_memmap_response->entry_count; ++i) {
        const struct limine_memmap_entry *limine_memmap_entry = limine_memmap_response->entries[i];
        if (limine_memmap_entry->type == LIMINE_MEMMAP_USABLE) {
            pmm_mark_range(limine_memmap_entry->base, limine_memmap_entry->length, true);
        }
    }
    pmm_mark_range(bitmap_phy, round_up(bitmap_size, PAGE_SIZE), false);
}

static void vmm_map(phy_t what, void *where, size_t size, enum memory_flags flags) {

}

void vmm_init(const struct limine_memmap_response *limine_memmap_response, const struct limine_kernel_address_response *limine_kernel_address_response) {            
    PML4 = phy_to_virt(pmm_alloc_page());
    memzero(PML4, PAGE_SIZE);

    for (uint64_t i = 0; i < limine_memmap_response->entry_count; ++i) {
        const struct limine_memmap_entry *limine_memmap_entry = limine_memmap_response->entries[i];
        switch (limine_memmap_entry->type) {
        case LIMINE_MEMMAP_USABLE:
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        case LIMINE_MEMMAP_FRAMEBUFFER:
            kprint("Mapping 0x%lx->0x%lx (0x%lx bytes)\n", limine_memmap_entry->base, limine_memmap_entry->base + limine_memmap_entry->length, limine_memmap_entry->length);
            vmm_map(limine_memmap_entry->base, phy_to_virt(limine_memmap_entry->base), limine_memmap_entry->length, M_W);
            break;
        case LIMINE_MEMMAP_RESERVED:
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        case LIMINE_MEMMAP_ACPI_NVS:
        case LIMINE_MEMMAP_BAD_MEMORY:
        case LIMINE_MEMMAP_KERNEL_AND_MODULES:  
        default:
            kprint("NOT Mapping 0x%lx->0x%lx (0x%lx bytes)\n", limine_memmap_entry->base, limine_memmap_entry->base + limine_memmap_entry->length, limine_memmap_entry->length);
            break;
        }
    }
    // FIXME: parse ELF, don't guess length/flags
    vmm_map(limine_kernel_address_response->physical_base, (void *)limine_kernel_address_response->virtual_base, 102400, M_W | M_X);

    __asm("mov %0,%%cr3" : : "r"(PML4) : "memory");
}
