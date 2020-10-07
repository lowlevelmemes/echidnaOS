#include <stdint.h>
#include <kernel.h>
#include <cio.h>

#define SUCCESS 0
#define EOF -1
#define FAILURE -2

#define MAX_STTY 1

static char* stty_names[] = { "stty0", "stty1", "stty2", "stty3" };

static char* com_devices[] = { ":://com1", ":://com2", ":://com3", ":://com4" };

static char* devices[MAX_STTY];

static int is_eof = 0;

int stty_io_read(uint32_t dev, uint64_t loc) {
    if (is_eof) {
        is_eof = 0;
        return -1;
    }

    int val = vfs_kread(devices[dev], 0);
    if (val == IO_NOT_READY) return IO_NOT_READY;
    switch (val) {
        case 0x0d:
            vfs_kwrite(devices[dev], 0, 0x0d);
            vfs_kwrite(devices[dev], 0, 0x0a);
            is_eof = 1;
            return 0x0a;
        case 0x08:
            vfs_kwrite(devices[dev], 0, 0x08);
            vfs_kwrite(devices[dev], 0, 0x20);
            vfs_kwrite(devices[dev], 0, 0x08);
            return 0x08;
    }
    vfs_kwrite(devices[dev], 0, val);
    return val;
}

int stty_io_write(uint32_t dev, uint64_t loc, uint8_t payload) {
    switch (payload) {
        case 0x0a:
            vfs_kwrite(devices[dev], 0, 0x0d);
            vfs_kwrite(devices[dev], 0, 0x0a);
            return SUCCESS;
        case 0x08:
            vfs_kwrite(devices[dev], 0, 0x08);
            vfs_kwrite(devices[dev], 0, 0x20);
            vfs_kwrite(devices[dev], 0, 0x08);
            return SUCCESS;
    }
    return vfs_kwrite(devices[dev], 0, payload);
}

int stty_io_wrapper(uint32_t dev, uint64_t loc, int type, uint8_t payload) {
    switch (type) {
        case DF_READ: stty_io_read(dev, loc);
        case DF_WRITE: stty_io_write(dev, loc, payload);
        default: return -1;
    }
}

void init_stty(void) {

    kputs("\nInitialising serial ttys...");
    
    for (int i = 0; i < MAX_STTY; i++) {
        kstrcpy(devices[i], com_devices[i]);
        kernel_add_device(stty_names[i], i, 0, &stty_io_wrapper);
        kputs("\nInitialised "); kputs(stty_names[i]);
    }

}
