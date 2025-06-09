#include "ps2.h"

#include "common.h"
#include "drivers/pic/ioapic.h"
#include "intel.h"

#define DATA_PORT 0x60
#define COMMAND_STATUS_PORT 0x64

typedef void f_ps2_driver(uint8_t data);

static f_ps2_driver *PORT1_DRIVER;
static f_ps2_driver *PORT2_DRIVER;

static uint8_t poll_reply() {
    while (!(inb(COMMAND_STATUS_PORT) & (1 << 0)));

    const uint8_t ret = inb(DATA_PORT);
    kprint("0x%x\n", ret);
    return ret;
}

static void send_command_noreply(uint8_t command) {
    outb(COMMAND_STATUS_PORT, command);
}

static uint8_t send_command(uint8_t command) {
    outb(COMMAND_STATUS_PORT, command);
    return inb(DATA_PORT);
}

static void send_to_device(uint8_t value) {
    while (inb(COMMAND_STATUS_PORT) & (1 << 1));
    outb(DATA_PORT, value);
}

static void write_config_byte(uint8_t config) {
    send_command_noreply(0x60);
    outb(DATA_PORT, config);
}

void ps2_init(void) {
    // Disable Devices
    send_command_noreply(0xAD);
    send_command_noreply(0xA7);

    // Clear buffer
    inb(DATA_PORT);

    // Read config byte
    uint8_t config = send_command(0x20);

    // Disable IRQs and translation
    config &= ~((1 << 6) | (1 << 0));
    write_config_byte(config);

    // Self Test
    const uint8_t self_test_result = send_command(0xAA);
    if (self_test_result != 0x55) {
        kprint("PS/2 self test failed with 0x%x.\nSkipping activation of PS/2 input devices.\n", self_test_result);
        return;
    }

    // Set config byte again for buggy chips
    write_config_byte(config);

    // Determine if there is a second channel (mouse)
    send_command_noreply(0xA8);
    config = send_command(0x20);

    bool has_kb = true;
    bool has_mouse = !(config & (1 << 5));

    // Self test port 1 (KB)
    const uint8_t port1_selftest_result = send_command(0xAB);
    if (port1_selftest_result) {
        panic("PS/2 port 1 (kb) faulty with code 0x%x\n", port1_selftest_result);
        has_kb = false;
    } else {
        send_command_noreply(0xAE);
    }

    // Self test port 2 (mouse)
    if (has_mouse) {
        const uint8_t port2_selftest_result = send_command(0xA9);
        if (port2_selftest_result) {
            panic("PS/2 port 2 (mouse) faulty with code 0x%x\n", port2_selftest_result);
            has_mouse = false;
        } else {
            send_command_noreply(0xA8);
        }
    }

    // Enable interrupts
    config = send_command(0x20);
    if (has_kb) config |= 0b01; 
    if (has_mouse) config |= 0b10;
    write_config_byte(config);

    // Reset devices and enable scanning
    if (has_kb) {
        kprint("KB RESET\n");
        send_to_device(0xFF);
        poll_reply();
        poll_reply();

        kprint("KB Enable Scanning\n");
        send_to_device(0xF4);
        poll_reply();
    }

    if (has_mouse) {
        kprint("MOUSE RESET\n");
        send_command_noreply(0xD4);
        send_to_device(0xFF);
        poll_reply();
        poll_reply();
        poll_reply();

        kprint("MOUSE Enable Scanning\n");
        send_command_noreply(0xD4);
        send_to_device(0xF4);
        poll_reply();
    }

    if (has_kb) ioapic_enable_isa_interrupt(IRQ_KB);
    if (has_mouse) ioapic_enable_isa_interrupt(IRQ_MOUSE);
}

void ps2_handle_port1_interrupt(void) {
    const uint8_t data = inb(DATA_PORT);
    if (PORT1_DRIVER) {
        PORT1_DRIVER(data);
    }
}

void ps2_handle_port2_interrupt(void) {
    const uint8_t data = inb(DATA_PORT);
    if (PORT2_DRIVER) {
        PORT2_DRIVER(data);
    }
}
