#include <kernel.h>
#include <cio.h>

void panic(const char *msg) {
    DISABLE_INTERRUPTS;
    text_set_text_palette(0x4E, current_tty);
    text_clear(current_tty);
    text_disable_cursor(current_tty);
    tty_kputs("\n!!! KERNEL PANIC !!!\n\nError info: ", current_tty);
    tty_kputs(msg, current_tty);
    tty_kputs("\n\nPress F1 to resume execution.\n", current_tty);

    while (port_in_b(0x60) != 0x05);
}
