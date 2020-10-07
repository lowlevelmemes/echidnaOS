PATH := $(shell pwd)/cross-root/bin:$(PATH)

.PHONY: all clean run

all: echidna.img

limine/limine-install:
	git clone https://github.com/limine-bootloader/limine.git --branch=v0.5 --depth=1
	cd limine && $(MAKE) limine-install

run:
	qemu-system-x86_64 -net none -enable-kvm -cpu host -hda echidna.img -m 2G -soundhw pcspk -debugcon stdio

shell/sh: shell/shell.c
	$(MAKE) -C shell

kernel/echidna.elf: $(KERNEL_FILES)
	$(MAKE) -C kernel

clean:
	$(MAKE) clean -C shell
	$(MAKE) clean -C kernel

echidna.img: limine/limine-install kernel/echidna.elf shell/sh
	dd bs=32768 count=0 seek=256 if=/dev/zero of=initramfs.img
	echfs-utils initramfs.img format 32768
	echfs-utils initramfs.img mkdir dev
	echfs-utils initramfs.img mkdir bin
	echfs-utils initramfs.img mkdir sys
	echfs-utils initramfs.img mkdir docs
	echfs-utils initramfs.img import ./shell/sh /sys/init
	echfs-utils initramfs.img import ./LICENSE.md /docs/license
	dd bs=32768 count=0 seek=8192 if=/dev/zero of=echidna.img
	parted -s echidna.img mklabel msdos
	parted -s echidna.img mkpart primary 2048s 100%
	echfs-utils -m -p0 echidna.img format 32768
	echfs-utils -m -p0 echidna.img import ./kernel/echidna.elf echidna.elf
	echfs-utils -m -p0 echidna.img import ./initramfs.img initramfs.img
	echfs-utils -m -p0 echidna.img import ./limine.cfg limine.cfg
	limine/limine-install limine/limine.bin echidna.img
