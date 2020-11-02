#include <stdint.h>
#include <stddef.h>
#include <kernel.h>

extern char rm_int_begin[], rm_int_end[];

void init_realmode(void) {
    memcpy(0xf000, rm_int_begin, rm_int_end - rm_int_begin);
}

void rm_int(uint8_t int_no, struct rm_regs *out_regs, struct rm_regs *in_regs) {
    void (*trampoline)(uint8_t, struct rm_regs *, struct rm_regs *) = (void*)0xf000;

    int r = escalate_privilege();

    map_PIC(0x08, 0x70);
    trampoline(int_no, out_regs, in_regs);
    map_PIC(0x20, 0x28);

    if (r)
        deescalate_privilege();
}
