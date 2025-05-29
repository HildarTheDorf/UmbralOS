#include "8259.h"

#include "intel.h"

#define PIC1 0x20
#define PIC1_COMMAND (PIC1 + 0)
#define PIC1_DATA (PIC1 + 1)

#define PIC2 0xA0
#define PIC2_COMMAND (PIC2 + 0)
#define PIC2_DATA (PIC2 + 1)

#define PIC_ICW1_ICW4 0x01
#define PIC_ICW1_INIT 0x10
#define PIC_ICW4_8086 0x01

#define PIC_READ_ISR 0x0B
#define PIC_EOI 0x20

void legacy_pic_init_and_disable(uint8_t master_offset, uint8_t slave_offset) {
    // ICW1: Begin INIT sequence, including ICW4
    outb(PIC1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    // ICW2: Map interrupts to IDT vectors
    outb(PIC1_DATA, master_offset);
    outb(PIC2_DATA, slave_offset);

    // ICW3: Configure cascade
    outb(PIC1_DATA, 4); // Slave PIC at IRQ2 (mask)
    outb(PIC2_DATA, 2); // Slave PIC is IRQ2 (value)

    // ICW4: Enable 8086 mode
    outb(PIC1_DATA, PIC_ICW4_8086);
    outb(PIC2_DATA, PIC_ICW4_8086);

    // Mask all interrupts
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}
