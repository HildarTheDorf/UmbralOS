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

OVMF_DIR := /usr/share/OVMF

.PHONY: all clean clean-nvs run run-bochs run-kvm run-uefi

all: umbralos.iso

clean:
	rm -rf build
	rm -rf iso_root
	rm -f umbralos.iso

clean-nvs:
	rm -f OVMF_VARS.fd

run: umbralos.iso
	$(QEMU) $(QEMU_FLAGS) -cpu qemu64,x2apic,smap,smep,umip -cdrom $<

run-bochs: umbralos.iso
	bochs

run-kvm: umbralos.iso
	$(QEMU) $(QEMU_FLAGS) -cpu host --enable-kvm -cdrom $<

run-uefi: umbralos.iso OVMF_VARS.fd
	$(QEMU) $(QEMU_FLAGS) \
		-drive if=pflash,file=$(OVMF_DIR)/OVMF_CODE_4M.fd,format=raw,readonly=on,unit=0 \
		-drive if=pflash,file=OVMF_VARS.fd,format=raw,unit=1 \
		-cpu host --enable-kvm -cdrom $<

OVMF_VARS.fd:
	cp $(OVMF_DIR)/OVMF_VARS_4M.fd OVMF_VARS.fd

build/%.s.o: src/%.s
	@mkdir -p $(@D)
	$(CC) $(ASMFLAGS) -c $< -o $@ 

build/%.c.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@ 

build/flanterm/%.c.o: flanterm/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@ 

iso_root/EFI/boot/% : limine/%
	@mkdir -p $(@D)
	cp $< $@

iso_root/boot/limine/% : limine/%
	@mkdir -p $(@D)
	cp $< $@

iso_root/boot/limine.conf: limine.conf.m4 iso_root/boot/umbralos.bin
	@mkdir -p $(@D)
	$(M4) $< > $@

iso_root/boot/umbralos.bin: linker.ld $(ALL_OBJECTS)
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) -T linker.ld $(ALL_OBJECTS) -o $@

umbralos.iso: iso_root/boot/umbralos.bin iso_root/boot/limine.conf $(BOOT_FILES)
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		-V UMBRALOS -m .gitkeep iso_root -quiet \
		-o $@
	limine bios-install --quiet $@
