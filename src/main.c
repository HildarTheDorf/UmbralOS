#include "acpi.h"
#include "common.h"
#include "gdt.h"
#include "interrupt.h"
#include "mm.h"
#include "security.h"
#include "serial.h"

#include "flanterm/flanterm.h"
#include "flanterm/backends/fb.h"

#define DEFAULT_STACK_SIZE 0x10000

[[gnu::used, gnu::section(".limine_requests.start")]]
static const LIMINE_REQUESTS_START_MARKER
[[gnu::used, gnu::section(".limine_requests")]]
static LIMINE_BASE_REVISION(3)

[[gnu::used, gnu::section(".limine_requests")]]
static struct limine_framebuffer_request limine_framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 1
};

[[gnu::used, gnu::section(".limine_requests")]]
static struct limine_hhdm_request limine_hhdm_request = {
    .id = LIMINE_HHDM_REQUEST
};

[[gnu::used, gnu::section(".limine_requests")]]
static struct limine_kernel_address_request limine_kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST
};

[[gnu::used, gnu::section(".limine_requests")]]
static struct limine_memmap_request limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,  
};

[[gnu::used, gnu::section(".limine_requests")]]
static struct limine_rsdp_request limine_rsdp_request = {
    .id = LIMINE_RSDP_REQUEST
};

[[gnu::used, gnu::section(".limine_requests.end")]]
static const LIMINE_REQUESTS_END_MARKER

static struct flanterm_context *flanterm_context;

void do_flanterm_write(const char *buf, size_t count) {
    flanterm_write(flanterm_context, buf, count);
}

[[noreturn]]
void main(void *stack_origin) {
    security_init();
    serial_init();
    kprint_configure(serial_write);

    load_gdt();
    load_idt();

    pmm_init(limine_memmap_request.response, (void *)limine_hhdm_request.response->offset);
    vmm_init(limine_memmap_request.response, limine_kernel_address_request.response);

    acpi_parse_rsdp(phy_to_virt((phy_t)limine_rsdp_request.response->address));
    configure_interrupts();

    if (limine_framebuffer_request.response->framebuffer_count > 0) {
        const struct limine_framebuffer *fb = limine_framebuffer_request.response->framebuffers[0];
        flanterm_context = flanterm_fb_init(
            nullptr, nullptr,
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
            0
        );
        kprint_configure(do_flanterm_write);
    }

    pmm_reclaim(limine_memmap_request.response, stack_origin, DEFAULT_STACK_SIZE);
#ifdef DEBUG_CHECKS
    pmm_zero();
#endif

    kprint("Boot Complete!\n");
    halt();
}
