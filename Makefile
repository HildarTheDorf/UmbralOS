CC := clang
CFLAGS := $(shell cat compile_flags.txt)
LD := ld.lld
LDFLAGS := --nostdlib -pie -z noexecstack --no-dynamic-linker
M4 := m4
QEMU := qemu-system-x86_64
QEMU_FLAGS := --no-reboot --no-shutdown -machine smm=off -d int -D qemu.log --serial stdio

ASM_SOURCES := gdt.s interrupt.s main.s
C_SOURCES := acpi.c common.c gdt.c intel.c interrupt.c main.c mm.c security.c serial.c
FLANTERM_SOURCES := flanterm.c backends/fb.c
ASM_OBJECTS := $(patsubst %.s,build/%.s.o,$(ASM_SOURCES))
C_OBJECTS := $(patsubst %.c,build/%.c.o,$(C_SOURCES))
FLANTERM_OBJECTS := $(patsubst %.c,build/flanterm/%.c.o,$(FLANTERM_SOURCES))

ALL_OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS) $(FLANTERM_OBJECTS)
ESP_DIRECTORY := iso_root/EFI/boot
LIMINE_DIRECTORY := iso_root/boot/limine
LIMINE_FILES := $(LIMINE_DIRECTORY)/limine-bios-cd.bin $(LIMINE_DIRECTORY)/limine-bios.sys $(LIMINE_DIRECTORY)/limine-uefi-cd.bin $(ESP_DIRECTORY)/BOOTIA32.EFI $(ESP_DIRECTORY)/BOOTX64.EFI

.PHONY: all clean run run-bochs run-kvm run-uefi

all: umbralos.iso

clean:
	rm -f $(ALL_OBJECTS)
	rm -f iso_root/boot/limine.conf
	rm -f iso_root/boot/umbralos.bin
	rm -f umbralos.iso

run: umbralos.iso
	$(QEMU) $(QEMU_FLAGS) -cpu qemu64,x2apic,smap,smep,umip -cdrom $<

run-bochs: umbralos.iso
	bochs

run-kvm: umbralos.iso
	$(QEMU) $(QEMU_FLAGS) -cpu host --enable-kvm -cdrom $<

run-uefi: umbralos.iso
	$(QEMU) $(QEMU_FLAGS) -cpu host --enable-kvm -bios /usr/share/ovmf/OVMF.fd -cdrom $<

build/%.s.o: src/%.s
	$(CC) $(ASMFLAGS) -c $^ -o $@ 

build/%.c.o: src/%.c
	$(CC) $(CFLAGS) -c $^ -o $@ 

build/flanterm/%.c.o: flanterm/%.c
	$(CC) $(CFLAGS) -c $^ -o $@ 

iso_root/boot/limine.conf: limine.conf.m4 iso_root/boot/umbralos.bin
	$(M4) $< > $@

iso_root/boot/umbralos.bin: linker.ld $(ALL_OBJECTS)
	$(LD) $(LDFLAGS) -T $^ -o $@

umbralos.iso: iso_root/boot/umbralos.bin iso_root/boot/limine.conf $(LIMINE_FILES)
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		-m .gitkeep iso_root -quiet -o $@
	limine bios-install --quiet $@
