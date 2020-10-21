PATH := $(shell pwd)/build/tools/host-binutils/bin:$(PATH)
PATH := $(shell pwd)/build/tools/host-gcc/bin:$(PATH)
IMGSIZE := 32768

.PHONY: all clean run

all: echidna.img

limine/limine-install:
	git clone https://github.com/limine-bootloader/limine.git --branch=v0.5.2 --depth=1
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
	rm -f echidna.img
	dd bs=$(IMGSIZE) count=0 seek=65536 if=/dev/zero of=echidna.img
	parted -s echidna.img mklabel msdos
	parted -s echidna.img mkpart primary 2048s 100%
	echfs-utils -m -p0 echidna.img format $(IMGSIZE)
	echfs-utils -m -p0 echidna.img mkdir dev
	echfs-utils -m -p0 echidna.img mkdir bin
	echfs-utils -m -p0 echidna.img mkdir sys
	echfs-utils -m -p0 echidna.img mkdir docs
	echfs-utils -m -p0 echidna.img import ./shell/sh /sys/init
	echfs-utils -m -p0 echidna.img import ./LICENSE.md /docs/license
	echfs-utils -m -p0 echidna.img import ./kernel/echidna.elf echidna.elf
	echfs-utils -m -p0 echidna.img import ./limine.cfg limine.cfg
	./copy-root-to-img.sh build/system-root/ echidna.img 0
	limine/limine-install limine/limine.bin echidna.img
