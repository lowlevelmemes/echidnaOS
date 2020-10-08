#include <stdint.h>
#include <kernel.h>

static char* tty_names[] = {
    "TTY 0", "TTY 1", "TTY 2", "TTY 3",
    "TTY 4", "TTY 5", "TTY 6", "TTY 7",
    "TTY 8", "TTY 9", "TTY 10", "TTY 11"
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
