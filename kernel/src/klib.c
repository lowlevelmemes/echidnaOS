#include <stdint.h>
#include <kernel.h>

int create_file_handle(int pid, file_handle_t handle) {
    int handle_n;

    // check for a free handle first
    for (int i = 0; i < task_table[pid]->file_handles_ptr; i++) {
        if (task_table[pid]->file_handles[i].free) {
            handle_n = i;
            goto load_handle;
        }
    }

    task_table[pid]->file_handles = krealloc(task_table[pid]->file_handles, (task_table[pid]->file_handles_ptr + 1) * sizeof(file_handle_t));
    handle_n = task_table[pid]->file_handles_ptr++;

load_handle:
    task_table[pid]->file_handles[handle_n] = handle;

    return handle_n;

}

int create_file_handle_v2(int pid, file_handle_v2_t handle) {
    int handle_n;

    // check for a free handle first
    for (int i = 0; i < task_table[pid]->file_handles_v2_ptr; i++) {
        if (task_table[pid]->file_handles_v2[i].free) {
            handle_n = i;
            goto load_handle;
        }
    }

    task_table[pid]->file_handles_v2 = krealloc(task_table[pid]->file_handles_v2, (task_table[pid]->file_handles_v2_ptr + 1) * sizeof(file_handle_v2_t));
    handle_n = task_table[pid]->file_handles_v2_ptr++;

load_handle:
    task_table[pid]->file_handles_v2[handle_n] = handle;

    return handle_n;

}

void kstrcpy(char* dest, char* source) {
    uint32_t i = 0;

    for ( ; source[i]; i++)
        dest[i] = source[i];

    dest[i] = 0;

    return;
}

int kstrcmp(char* dest, char* source) {
    uint32_t i = 0;

    for ( ; dest[i] == source[i]; i++)
        if ((!dest[i]) && (!source[i])) return 0;

    return 1;
}

int kstrncmp(char* dest, char* source, uint32_t len) {
    uint32_t i = 0;

    for ( ; i < len; i++)
        if (dest[i] != source[i]) return 1;

    return 0;
}

uint32_t kstrlen(char* str) {
    uint32_t len;

    for (len = 0; str[len]; len++);

    return len;
}

struct alloc_metadata {
    size_t pages;
    size_t size;
};

void* kalloc(size_t size) {
    size_t page_count = DIV_ROUNDUP(size, PAGE_SIZE);

    char *ptr = pmm_allocz(page_count + 1);

    if (!ptr)
        return NULL;

    struct alloc_metadata *metadata = (void*)ptr;
    ptr += PAGE_SIZE;

    metadata->pages = page_count;
    metadata->size = size;

    return ptr;
}

void* krealloc(void* ptr, size_t new_size) {
    /* check if 0 */
    if (!ptr)
        return kalloc(new_size);

    /* Reference metadata page */
    struct alloc_metadata *metadata = (void*)((char *)ptr - PAGE_SIZE);

    if (DIV_ROUNDUP(metadata->size, PAGE_SIZE) == DIV_ROUNDUP(new_size, PAGE_SIZE)) {
        metadata->size = new_size;
        return ptr;
    }

    void *new_ptr = kalloc(new_size);
    if (new_ptr == NULL)
        return NULL;

    if (metadata->size > new_size)
        /* Copy all the data from the old pointer to the new pointer,
         * within the range specified by `size`. */
        memcpy(new_ptr, ptr, new_size);
    else
        memcpy(new_ptr, ptr, metadata->size);

    kfree(ptr);

    return new_ptr;
}

void kfree(void* ptr) {
    struct alloc_metadata *metadata = (void*)((char *)ptr - PAGE_SIZE);

    pmm_free((void *)metadata, metadata->pages + 1);
}

uint64_t power(uint64_t x, uint64_t y) {
    uint64_t res;
    for (res = 1; y; y--)
        res *= x;
    return res;
}

void kputs(const char* string) {

    #ifdef _SERIAL_KERNEL_OUTPUT_
      for (int i = 0; string[i]; i++) {
          if (string[i] == '\n') {
              com_io_wrapper(0, 0, 1, 0x0d);
              com_io_wrapper(0, 0, 1, 0x0a);
          } else
              com_io_wrapper(0, 0, 1, string[i]);
      }
    #else
      tty_kputs(string, 0);
    #endif

    return;
}

void tty_kputs(const char* string, uint8_t which_tty) {
    uint32_t i;
    for (i = 0; string[i]; i++)
        text_putchar(string[i], which_tty);
    return;
}

void knputs(const char* string, uint32_t count) {

    #ifdef _SERIAL_KERNEL_OUTPUT_
      for (int i = 0; i < count; i++)
          com_io_wrapper(0, 0, 1, string[i]);
    #else
      tty_knputs(string, count, 0);
    #endif

    return;
}

void tty_knputs(const char* string, uint32_t count, uint8_t which_tty) {
    uint32_t i;
    for (i = 0; i < count; i++)
        text_putchar(string[i], which_tty);
    return;
}

void kuitoa(uint64_t x) {
    uint8_t i;
    char buf[21] = {0};

    if (!x) {
        kputs("0");
        return;
    }

    for (i = 19; x; i--) {
        buf[i] = (x % 10) + 0x30;
        x = x / 10;
    }

    i++;
    kputs(buf + i);

    return;
}

void tty_kuitoa(uint64_t x, uint8_t which_tty) {
    uint8_t i;
    char buf[21] = {0};

    if (!x) {
        tty_kputs("0", which_tty);
        return;
    }

    for (i = 19; x; i--) {
        buf[i] = (x % 10) + 0x30;
        x = x / 10;
    }

    i++;
    tty_kputs(buf + i, which_tty);

    return;
}

static const char hex_to_ascii_tab[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

void kxtoa(uint64_t x) {
    uint8_t i;
    char buf[17] = {0};

    if (!x) {
        kputs("0x0");
        return;
    }

    for (i = 15; x; i--) {
        buf[i] = hex_to_ascii_tab[(x % 16)];
        x = x / 16;
    }

    i++;
    kputs("0x");
    kputs(buf + i);

    return;
}

void tty_kxtoa(uint64_t x, uint8_t which_tty) {
    uint8_t i;
    char buf[17] = {0};

    if (!x) {
        tty_kputs("0x0", which_tty);
        return;
    }

    for (i = 15; x; i--) {
        buf[i] = hex_to_ascii_tab[(x % 16)];
        x = x / 16;
    }

    i++;
    tty_kputs("0x", which_tty);
    tty_kputs(buf + i, which_tty);

    return;
}
