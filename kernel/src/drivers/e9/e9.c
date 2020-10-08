#include <stdint.h>
#include <kernel.h>
#include <cio.h>

#define PORT 0xe9
#define MAGIC 0xe9

static int e9_io_read(void) {
    return port_in_b(PORT);
}

static int e9_io_write(uint8_t payload) {
    port_out_b(PORT, payload);
    return 0;
}

/* Bochs Debug Console
 * READ: returns 0xe9
 * WRITE: writes payload to console */
int e9_io_wrapper(uint32_t unused0, uint64_t unused1, int type, uint8_t payload) {
    switch (type) {
        case DF_READ: return e9_io_read();
        case DF_WRITE: return e9_io_write(payload);
        default: return -1;
    }
}

// checks if bochs debug console is present
static inline int detect_e9(void) {
    return port_in_b(PORT) == MAGIC;
}

void init_e9(void) {
    if (detect_e9()) {
        kernel_add_device("E9", 0, 0, &e9_io_wrapper);
    }
}