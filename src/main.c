#include "common.h"
#include "gdt.h"
#include "intel.h"
#include "interrupt.h"
#include "mm.h"
#include "security.h"

#include "drivers/acpi/acpi.h"
#include "drivers/ps2.h"
#include "drivers/serial.h"

#include "flanterm.h"
#include "flanterm_backends/fb.h"

#define DEFAULT_STACK_SIZE 0x10000

[[gnu::used, gnu::section(".limine_requests.start")]]
static const uint64_t limine_start[] = LIMINE_REQUESTS_START_MARKER;
[[gnu::used, gnu::section(".limine_requests")]]
static uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

[[gnu::used, gnu::section(".limine_requests")]]
static struct limine_framebuffer_request limine_framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 1
};

[[gnu::used, gnu::section(".limine_requests")]]
static struct limine_hhdm_request limine_hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID
};

[[gnu::used, gnu::section(".limine_requests")]]
static struct limine_executable_address_request limine_executable_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID
};

[[gnu::used, gnu::section(".limine_requests")]]
static struct limine_memmap_request limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,  
};

[[gnu::used, gnu::section(".limine_requests")]]
static struct limine_rsdp_request limine_rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID
};

[[gnu::used, gnu::section(".limine_requests.end")]]
static const uint64_t limine_end[] = LIMINE_REQUESTS_END_MARKER;

static struct flanterm_context *flanterm_context;

static void do_flanterm_write(const char *buf, size_t count) {
    serial_write(buf, count);
    flanterm_write(flanterm_context, buf, count);
}

static void fp_init(void) {
    uint64_t cr0;
    __asm volatile("mov %%cr0,%0" : "=r"(cr0));
    // x87 settings
    // Set EM = 0, MP = 1 as per SDM Table 11-3, and to enable SSE
    // Set NE = 1 to use native handling of x87 errors
    // Set TS = 1 to enable lazy saving of the x87/SSE state
    cr0 &= ~CR0_EM;
    cr0 |= CR0_NE | CR0_TS | CR0_MP;
    __asm("mov %0,%%cr0" : : "r"(cr0));

    uint64_t cr4;
    __asm volatile("mov %%cr4,%0" : "=r"(cr4));
    // OSFXSR: Enable (most) SSE/2/3/4 instructions
    //         Ensure FXSAVE/FXRSTOR save XMM state
    // OSXMMEXCEPT: Enable #XM exceptions (instead of #UD)
    cr4 |= CR4_OSXMMEXCPT | CR4_OSFXSR;
    __asm("mov %0,%%cr4" : : "r"(cr4));
}

[[noreturn]]
void main(void *stack_origin) {
    security_init();
    serial_early_init();
    kprint_configure(serial_write);
    kprint("Kernel Base: 0x%lx\n", limine_executable_address_request.response->virtual_base);

    load_gdt();
    load_idt();

    pmm_init(limine_memmap_request.response, (void *)limine_hhdm_request.response->offset);
    vmm_init(limine_memmap_request.response, limine_executable_address_request.response);

    if (limine_framebuffer_request.response->framebuffer_count > 0) {
        const struct limine_framebuffer *fb = limine_framebuffer_request.response->framebuffers[0];
        flanterm_context = flanterm_fb_init(
            kalloc, kfree,
            fb->address,
            fb->width, fb->height, fb->pitch,
            fb->red_mask_size, fb->red_mask_shift,
            fb->green_mask_size, fb->green_mask_shift,
            fb->blue_mask_size, fb->blue_mask_shift,
            nullptr,
            nullptr, nullptr,
            nullptr, nullptr,
            nullptr, nullptr,
            nullptr, 0, 0, 1,
            0, 0,
            0,
            FLANTERM_FB_ROTATE_0
        );
        kprint_configure(do_flanterm_write);
    }

    if (!limine_rsdp_request.response) panic("No RSDP response from limine");
    acpi_parse_rsdp(limine_rsdp_request.response->address);
    enable_interrupts();

    pmm_reclaim(limine_memmap_request.response, stack_origin, DEFAULT_STACK_SIZE);
#ifdef DEBUG_CHECKS
    pmm_zero();
#endif

    serial_init();
    if (acpi_has_legacy_feature(ACPI_LEGACY_FEATURE_i8042)) { ps2_init(); }
    fp_init();

    __asm("int $3");
    kprint("Boot Complete!\n");
    halt();
}
