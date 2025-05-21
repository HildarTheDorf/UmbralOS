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

ASM_OBJECTS := $(ASM_SOURCES:%.s=build/%.s.o)
C_OBJECTS := $(C_SOURCES:%.c=build/%.c.o)
FLANTERM_OBJECTS := $(FLANTERM_SOURCES:%.c=build/flanterm/%.c.o)
ALL_OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS) $(FLANTERM_OBJECTS)

ESP_DIR := iso_root/EFI/boot
LIMINE_BOOT_DIR := iso_root/boot/limine
ESP_BINARIES := BOOTIA32.EFI BOOTX64.EFI
LIMINE_BINARIES := limine-bios-cd.bin limine-bios.sys limine-uefi-cd.bin
BOOT_FILES := $(ESP_BINARIES:%=$(ESP_DIR)/%) $(LIMINE_BINARIES:%=$(LIMINE_BOOT_DIR)/%)

.PHONY: all clean run run-bochs run-kvm run-uefi

all: umbralos.iso

clean:
	rm -f $(ALL_OBJECTS)
	rm -f $(BOOT_FILES)
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

iso_root/EFI/boot/% : /usr/local/share/limine/%
	cp $< $@

iso_root/boot/limine/% : /usr/local/share/limine/%
	cp $< $@

iso_root/boot/limine.conf: limine.conf.m4 iso_root/boot/umbralos.bin
	$(M4) $< > $@

iso_root/boot/umbralos.bin: linker.ld $(ALL_OBJECTS)
	$(LD) $(LDFLAGS) -T $^ -o $@

umbralos.iso: iso_root/boot/umbralos.bin iso_root/boot/limine.conf $(BOOT_FILES)
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		-m .gitkeep iso_root -quiet -o $@
	limine bios-install --quiet $@
