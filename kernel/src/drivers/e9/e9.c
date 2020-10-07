#include <stdint.h>
#include <kernel.h>
#include <cio.h>

#define PORT 0xe9
#define MAGIC 0xe9

/* Bochs Debug Console
 * READ: returns 0xe9
 * WRITE: writes payload to console */
int e9_io_wrapper(uint32_t unused0, uint64_t unused1, int type, uint8_t payload) {
    if (type == DF_READ) {
        // this should always return 0xe9
        return port_in_b(PORT);
    } else {
        // prints a character to the bochs debug console
        port_out_b(PORT, payload);
        return 0;
    }
}

// checks if bochs debug console is present
static inline int detect_e9(void) {
    return port_in_b(PORT) == MAGIC;
}

void init_e9(void) {
    if (detect_e9()) {
        kernel_add_device("e9", 0, 0, &e9_io_wrapper);
    }
}