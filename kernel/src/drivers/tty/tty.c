#include <stdint.h>
#include <kernel.h>

static char* tty_names[] = {
    "tty 0", "tty 1", "tty 2", "tty 3",
    "tty 4", "tty 5", "tty 6", "tty 7",
    "tty 8", "tty 9", "tty 10", "tty 11"
};

int tty_io_wrapper(uint32_t tty, uint64_t unused, int type, uint8_t payload) {

    if (type == DF_WRITE) {
        text_putchar(payload, tty);
        return 0;
    } else if (type == DF_READ)
        return keyboard_fetch_char(tty);

}

void init_tty_drv(void) {

    for (int i = 0; i < KRNL_TTY_COUNT; i++)
        kernel_add_device(tty_names[i], i, 0, &tty_io_wrapper);

    return;

}
