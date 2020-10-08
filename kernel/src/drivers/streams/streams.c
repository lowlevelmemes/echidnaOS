#include <stdint.h>
#include <kernel.h>

#define EOF -1

int stdin_io_wrapper(uint32_t unused, uint64_t loc, int type, uint8_t payload) {
    if (type == DF_READ)
        return vfs_kread(task_table[current_task]->stdin, loc);
    else if (type == DF_WRITE)
        return 0;
}

int stdout_io_wrapper(uint32_t unused, uint64_t loc, int type, uint8_t payload) {
    if (type == DF_READ)
        return 0;
    else if (type == DF_WRITE)
        return vfs_kwrite(task_table[current_task]->stdout, loc, payload);
}

int stderr_io_wrapper(uint32_t unused, uint64_t loc, int type, uint8_t payload) {
    if (type == DF_READ)
        return 0;
    else if (type == DF_WRITE)
        return vfs_kwrite(task_table[current_task]->stderr, loc, payload);
}

int null_io_wrapper(uint32_t unused, uint64_t loc, int type, uint8_t payload) {
    if (type == DF_READ)
        return EOF;
    else if (type == DF_WRITE)
        return 0;
}

int zero_io_wrapper(uint32_t unused, uint64_t loc, int type, uint8_t payload) {
    if (type == DF_READ)
        return '\0';
    else if (type == DF_WRITE)
        return 0;
}

void init_streams(void) {
    kernel_add_device("Standard In", 0, 0, &stdin_io_wrapper);
    kernel_add_device("Standard Out", 0, 0, &stdout_io_wrapper);
    kernel_add_device("Standard Error", 0, 0, &stderr_io_wrapper);
    kernel_add_device("Null", 0, 0, &null_io_wrapper);
    kernel_add_device("Zero", 0, 0, &zero_io_wrapper);
    return;
}
