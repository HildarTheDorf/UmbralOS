#include "mm.h"
#include "common.h"
#include "intel.h"

#include <elf.h>
#include <limine.h>
#include <limits.h>

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

static enum memory_flags elf_to_memory_flags(Elf64_Word elf_flags) {
    enum memory_flags flags = M_NONE;
    if (elf_flags & PF_W) flags |= M_W;
    if (elf_flags & PF_X) flags |= M_X;
    return flags;
}

static void pmm_addr_to_bitmap_location(phy_t page, size_t *bitmap_index, uint8_t *bitmap_mask) {
    if (page % PAGE_SIZE) panic("addr_to_bitmap_location: addr 0x%lx not aligned\n", page);
    const size_t page_idx = page / PAGE_SIZE;

    if (page_idx >= PMM_DATA.num_pages) panic("page_to_bitmap_location: page 0x%lx out of range\n", page);

    const uint8_t bit_index = page_idx % CHAR_BIT;
    *bitmap_index = page_idx / CHAR_BIT;
    *bitmap_mask = 1 << bit_index;
}

static void pmm_mark_page(phy_t page, bool usable) {
    size_t bitmap_index;
    uint8_t bitmap_mask;
    pmm_addr_to_bitmap_location(page, &bitmap_index, &bitmap_mask);
    if (usable) {
        PMM_DATA.bitmap[bitmap_index] |= bitmap_mask;
    } else {
        PMM_DATA.bitmap[bitmap_index] &= ~bitmap_mask;
    }
}

static bool pmm_query_page(phy_t page) {
    size_t bitmap_index;
    uint8_t bitmap_mask;
    pmm_addr_to_bitmap_location(page, &bitmap_index, &bitmap_mask);
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
    memzero(PMM_DATA.bitmap, bitmap_size);

    for (uint64_t i = 0; i < limine_memmap_response->entry_count; ++i) {
        const struct limine_memmap_entry *limine_memmap_entry = limine_memmap_response->entries[i];
        if (limine_memmap_entry->type == LIMINE_MEMMAP_USABLE) {
            pmm_mark_range(limine_memmap_entry->base, limine_memmap_entry->length, true);
        }
    }

    pmm_mark_range(bitmap_phy, round_up(bitmap_size, PAGE_SIZE), false);
}

void pmm_reclaim(const struct limine_memmap_response *limine_memmap_response, void *stack_origin, size_t stack_size) {
    for (uint64_t i = 0; i < limine_memmap_response->entry_count; ++i) {
        const struct limine_memmap_entry *limine_memmap_entry = limine_memmap_response->entries[i];
        switch (limine_memmap_entry->type) {
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            pmm_mark_range(limine_memmap_entry->base, limine_memmap_entry->length, true);
            break;
        default:
            break;
        }
    }

    const uintptr_t stack_top = (uintptr_t)stack_origin;
    const uintptr_t stack_bottom = stack_top - stack_size;
    pmm_mark_range(stack_bottom - (uintptr_t)pHHDM, stack_size, false);
}

#ifdef DEBUG_CHECKS
void pmm_zero(void) {
    for (size_t i = 0; i < PMM_DATA.num_pages; ++i) {
        if (pmm_query_page(i * PAGE_SIZE)) {
            memzero(phy_to_virt(i * PAGE_SIZE), PAGE_SIZE);
        }
    }
}
#endif

static void *vmm_ensure_present(struct page_directory_entry_subdirectory *entry) {
    if (!entry->p) {
        const phy_t page = pmm_alloc_page();
        memzero(phy_to_virt(page), PAGE_SIZE);

        entry->xd = 0;
        entry->addr = page >> 12;
        entry->ps = 0;
        entry->a = 0;
        entry->pcd = 0;
        entry->pwt = 0;
        entry->us = 0;
        entry->rw = 1;
        entry->p = 1;
    }

    return phy_to_virt(entry->addr << 12);
}

static void vmm_map_page(phy_t what, void *where, enum memory_flags flags) {
    if (what % PAGE_SIZE) panic("vmm_map_page: what not aligned\n");
    if ((uintptr_t)where % PAGE_SIZE) panic("vmm_map_page: where not aligned\n");
    if (flags & M_W && flags & M_X) panic("vmm_map_page: W|X memory is forbidden");

    const uint16_t pml4_index = (((uintptr_t)where) >> 39) & 0x1FF;
    struct page_directory_entry_subdirectory *pdpt = vmm_ensure_present(&PML4[pml4_index]);

    const uint16_t pdpt_index = (((uintptr_t)where) >> 30) & 0x1FF;
    struct page_directory_entry_subdirectory *pd = vmm_ensure_present(&pdpt[pdpt_index]);

    const uint16_t pd_index = (((uintptr_t)where) >> 21) & 0x1FF;
    struct page_table_entry *pt = vmm_ensure_present(&pd[pd_index]);

    const uint16_t pt_index = (((uintptr_t)where) >> 12) & 0x1FF;
    pt[pt_index].xd = !(flags & M_X);
    pt[pt_index].pk = 0x0;
    pt[pt_index].addr = what >> 12;
    pt[pt_index].g = 0;
    pt[pt_index].pat = 0;
    pt[pt_index].a = 0;
    pt[pt_index].d = 0;
    pt[pt_index].pcd = 0;
    pt[pt_index].pwt = 0;
    pt[pt_index].us = !!(flags & M_U);
    pt[pt_index].rw = !!(flags & M_W);
    pt[pt_index].p = 1;
}


void vmm_map(phy_t what, void *where, size_t size, enum memory_flags flags) {
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        vmm_map_page(what + i, (void *)((uintptr_t)where + i), flags);
    }
}   

void vmm_init(const struct limine_memmap_response *limine_memmap_response, const struct limine_kernel_address_response *limine_kernel_address_response) {            
    PML4 = phy_to_virt(pmm_alloc_page());
    memzero(PML4, PAGE_SIZE);

    for (uint64_t i = 0; i < limine_memmap_response->entry_count; ++i) {
        const struct limine_memmap_entry *limine_memmap_entry = limine_memmap_response->entries[i];
     
        switch (limine_memmap_entry->type) {
        case LIMINE_MEMMAP_USABLE:
        case LIMINE_MEMMAP_RESERVED:
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            const phy_t what = round_down(limine_memmap_entry->base, PAGE_SIZE);
            const size_t size = round_up(limine_memmap_entry->base + limine_memmap_entry->length, PAGE_SIZE) - limine_memmap_entry->base;
            vmm_map(what, phy_to_virt(what), size, M_W);
            break;
        case LIMINE_MEMMAP_ACPI_NVS:
        case LIMINE_MEMMAP_BAD_MEMORY:
        case LIMINE_MEMMAP_KERNEL_AND_MODULES:
        case LIMINE_MEMMAP_FRAMEBUFFER:  
        default:
            break;
        }
    }

    const Elf64_Ehdr *elf_header = (void *)limine_kernel_address_response->virtual_base;
    const Elf64_Phdr *elf_phdrs = (void *)(limine_kernel_address_response->virtual_base + elf_header->e_phoff);
    for (Elf64_Half i = 0; i < elf_header->e_phnum; ++i) {      
        const Elf64_Phdr *phdr = &elf_phdrs[i];
        if (phdr->p_type == PT_LOAD) {
            const Elf64_Addr base = round_down(phdr->p_vaddr, PAGE_SIZE);
            const Elf64_Addr end = round_up(phdr->p_vaddr + phdr->p_memsz, PAGE_SIZE);
            vmm_map(
                limine_kernel_address_response->physical_base + base,
                (void *)(limine_kernel_address_response->virtual_base + base),
                end - base, elf_to_memory_flags(phdr->p_flags)
            );
        }
    }

    for (Elf64_Half i = 0; i < elf_header->e_phnum; ++i) {      
        const Elf64_Phdr *phdr = &elf_phdrs[i];
        if (phdr->p_type == PT_GNU_RELRO) {
            const Elf64_Addr base = round_down(phdr->p_vaddr, PAGE_SIZE);
            const Elf64_Addr end = round_up(phdr->p_vaddr + phdr->p_memsz, PAGE_SIZE);
            vmm_map(
                limine_kernel_address_response->physical_base + base,
                (void *)(limine_kernel_address_response->virtual_base + base),
                end - base, elf_to_memory_flags(phdr->p_flags)
            );
        }
    }

    __asm("mov %0,%%cr3" : : "r"((uintptr_t)PML4 - (uintptr_t)pHHDM) : "memory");
}
