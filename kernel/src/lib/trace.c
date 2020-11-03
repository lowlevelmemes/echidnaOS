#include <stddef.h>
#include <kernel.h>
#include <symlist.h>

char *trace_address(size_t *off, size_t addr) {
    for (size_t i = 0; ; i++) {
        if (symlist[i].addr >= addr) {
            *off = addr - symlist[i-1].addr;
            return symlist[i-1].name;
        }
    }
}

void print_stacktrace(int type, size_t *base_ptr) {
    if (base_ptr == NULL) {
        asm volatile (
            "mov %0, ebp;"
            : "=g"(base_ptr)
        );
    }
    kprint(type, "Stacktrace:");
    for (;;) {
        size_t old_bp = base_ptr[0];
        size_t ret_addr = base_ptr[1];
        if (!ret_addr)
            break;
        size_t off;
        char *name = trace_address(&off, ret_addr);
        kprint(type, "  [%x] <%s+%x>", ret_addr, name, off);
        if (!old_bp)
            break;
        base_ptr = (void*)old_bp;
    }
}
