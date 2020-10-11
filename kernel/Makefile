CC = i686-echidnaos-gcc
PREFIX =

C_FILES = init.c $(shell find src -type f -name '*.c')
H_FILES = $(shell find . -type f -name '*.h')
ASM_FILES = $(shell find asm -type f -name '*.asm')
REAL_FILES = $(shell find blobs -type f -name '*.real')
OBJ = $(C_FILES:.c=.o) $(ASM_FILES:.asm=.o)
BINS = $(REAL_FILES:.real=.bin)

CHARDFLAGS = \
    -std=gnu99 \
    -fplan9-extensions \
    -masm=intel \
    -ffreestanding \
    -fno-stack-protector \
    -mno-80387 \
    -mno-mmx \
    -I./include
CFLAGS = -O2 -Wall -Wextra -pipe

LDHARDFLAGS = \
    -lgcc \
    -static-libgcc \
    -nostdlib \
    -no-pie \
    -static \
    -Tlinker.ld
LDFLAGS = -O2

echidna.elf: $(BINS) $(OBJ) $(H_FILES)
	$(CC) $(OBJ) $(LDFLAGS) $(LDHARDFLAGS) -o $@

%.o: %.c
	$(CC) $(CHARDFLAGS) $(CFLAGS) -c $< -o $@

%.bin: %.real
	nasm $< -f bin -o $@

%.o: %.asm
	nasm $< -f elf32 -o $@

clean:
	rm -f $(OBJ) $(BINS) echidna.elf

install:
	cp echidna.elf $(PREFIX)/
