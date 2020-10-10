#include <stdint.h>
#include <kernel.h>
#include <cio.h>

static uint16_t div = 0;
static uint32_t freq = 0;
static int get_freq = 4;

int pcspk_io_read(void) {
    return div;
}

int pcspk_io_write(uint8_t payload) {
    freq *= 0x100;
    freq += payload;

    if (!(--get_freq)) {

        get_freq = 4;

        if (!freq) {
            port_out_b(0x61, (port_in_b(0x61) & 0b11111100));
            div = 0;
            return 0;
        }

        port_out_b(0x43, 0xb6);

        div = 1193180 / freq;

        port_out_b(0x42, (div & 0xff));
        port_out_b(0x42, (div & 0xff00) >> 8);
        
        port_out_b(0x61, (port_in_b(0x61) | 0b00000011));
        
        freq = 0;
        return 0;
    }

    return -1;
}

int pcspk_io_wrapper(uint32_t unused0, uint64_t unused1, int type, uint8_t payload) {
    switch (type) {
        case DF_READ: return pcspk_io_read();
        case DF_WRITE: return pcspk_io_write(payload);
        default: return -1;
    }
}

void init_pcspk(void) {
    kernel_add_device("pcspkr", 0, 0, &pcspk_io_wrapper);
    return;
}
