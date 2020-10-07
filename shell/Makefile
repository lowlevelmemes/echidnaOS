CC = i686-echidnaos-gcc
PREFIX =

C_FILES = shell.c
OBJ_FILES = $(C_FILES:.c=.o)

CFLAGS = -O2 -Wall -Wextra

sh: $(OBJ_FILES)
	$(CC) -o sh $(OBJ_FILES)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILES) sh

install:
	mkdir -p $(PREFIX)/bin
	cp sh $(PREFIX)/bin/
