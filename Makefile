CC := clang
CFLAGS := $(shell cat src/compile_flags.txt)
LD := ld.lld
LDFLAGS := --nostdlib -pie

ESP_DIRECTORY := iso_root/EFI/boot
LIMINE_DIRECTORY := iso_root/boot/limine
LIMINE_FILES := ${LIMINE_DIRECTORY}/limine-bios-cd.bin ${LIMINE_DIRECTORY}/limine-bios.sys ${LIMINE_DIRECTORY}/limine-uefi-cd.bin ${ESP_DIRECTORY}/BOOTIA32.EFI ${ESP_DIRECTORY}/BOOTX64.EFI

.PHONY: all clean run

all: umbralos.iso

clean:
	rm -f build/*
	rm -f iso_root/boot/umbralos.bin
	rm -f umbralos.iso

run: umbralos.iso
	qemu-system-x86_64 --no-reboot --no-shutdown -machine smm=off -d int -D qemu.log -cdrom $<

build/%.s.o: src/%.s
	${CC} ${ASMFLAGS} -c $^ -o $@ 

build/%.o: src/%.c
	${CC} ${CFLAGS} -c $^ -o $@ 

iso_root/boot/umbralos.bin: build/main.o build/main.s.o
	${LD} ${LDFLAGS} $^ -o $@

umbralos.iso: iso_root/boot/umbralos.bin iso_root/boot/limine.conf ${LIMINE_FILES}
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -quiet -o $@
	limine bios-install --quiet $@
