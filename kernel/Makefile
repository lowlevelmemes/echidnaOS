CC = i686-echidnaos-gcc
PREFIX =

C_FILES = init.c $(shell find src -type f -name '*.c')
H_FILES = $(shell find . -type f -name '*.h')
ASM_FILES = $(shell find asm -type f -name '*.asm')
OBJ = $(C_FILES:.c=.o) $(ASM_FILES:.asm=.o)

CHARDFLAGS = \
    -std=gnu11 \
    -fplan9-extensions \
    -fno-omit-frame-pointer \
    -masm=intel \
    -ffreestanding \
    -fno-stack-protector \
    -mno-80387 \
    -mno-mmx \
    -I./include
CFLAGS = -g -O3 -Wall -Wextra -pipe

LDHARDFLAGS = \
    -lgcc \
    -static-libgcc \
    -nostdlib \
    -no-pie \
    -static \
    -Tlinker.ld
LDFLAGS = -g -O3

.PHONY: clean install

echidna.elf: $(OBJ) $(H_FILES)
	echo '#include <symlist.h>' > symlist.gen
	echo '__attribute__((section(".symlist")))' >> symlist.gen
	echo 'struct symlist_t symlist[] = {{0xffffffff,""}};' >> symlist.gen
	$(CC) -x c $(CHARDFLAGS) $(CFLAGS) -c symlist.gen -o symlist.o
	$(CC) $(OBJ) symlist.o $(LDFLAGS) $(LDHARDFLAGS) -o $@
	./gensyms.sh
	$(CC) -x c $(CHARDFLAGS) $(CFLAGS) -c symlist.gen -o symlist.o
	$(CC) $(OBJ) symlist.o $(LDFLAGS) $(LDHARDFLAGS) -o $@

%.o: %.c
	$(CC) $(CHARDFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm $< -g -f elf32 -o $@

clean:
	rm -f $(OBJ) $(BINS) echidna.elf

install:
	cp echidna.elf $(PREFIX)/
