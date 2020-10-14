#include <stdint.h>
#include <kernel.h>
#include <stivale.h>

#define SUCCESS 0
#define EOF -1
#define FAILURE -2

static uint8_t *initramfs;
static size_t   initramfs_size;

int initramfs_io_wrapper(uint32_t dev, uint64_t loc, int type, uint8_t payload) {
    if (loc >= initramfs_size)
        return EOF;
    if (type == DF_READ) {
        return (int)initramfs[loc];
    }
    else if (type == DF_WRITE) {
        initramfs[loc] = payload;
        return SUCCESS;
    }
}

void init_initramfs(struct stivale_struct *stivale_struct) {

    kputs("\nInitialising initramfs driver...\n");

    struct stivale_module *module = (void*)(uintptr_t)stivale_struct->modules;

    initramfs = (void*)(uintptr_t)module->begin;
    initramfs_size = (size_t)(module->end - module->begin);

    kernel_add_device("initrd", 0, initramfs_size, &initramfs_io_wrapper);

    kputs("\nInitialised initramfs.");

}
