#include <stdint.h>
#include <stddef.h>
#include <kernel.h>

struct idt_entry {
    uint16_t offset_low16;
    uint16_t cs;
    uint8_t zero;
    uint8_t attributes;
    uint16_t offset_high16;
} __attribute__((packed));

struct idt_ptr {
    uint16_t size;
    /* Start address */
    uint32_t address;
} __attribute__((packed));

static struct idt_entry idt[256] = {0};
extern void *int_thunks[];
extern void *exception_thunks[];

static void register_interrupt_handler(size_t vec, uint16_t cs, void *handler, uint8_t type) {
    uint32_t p = (uint32_t)handler;

    idt[vec].offset_low16 = (uint16_t)p;
    idt[vec].cs = cs;
    idt[vec].zero = 0;
    idt[vec].attributes = type;
    idt[vec].offset_high16 = (uint16_t)(p >> 16);
}

int escalate_privilege(void) {
    int ret;
    asm volatile ("mov %0, ds" : "=r"(ret) ::"memory");
    if (!(ret & 0x03))
        return 0;

    asm volatile ("int 0x90":::"eax", "memory");
    asm volatile (
        "mov ds, %0;"
        "mov es, %0;"
        "mov fs, %0;"
        "mov gs, %0;"
        :: "r"(0x10) : "memory"
    );
    return 1;
}

void deescalate_privilege(void) {
    asm volatile (
        "mov ds, %0;"
        "mov es, %0;"
        "mov fs, %0;"
        "mov gs, %0;"
        "mov eax, esp;"
        "push 0x21;"
        "push eax;"
        "pushf;"
        "push 0x19;"
        "push OFFSET 1f;"
        "iretd;"
        "1:"
        :: "r"(0x21) : "eax", "memory"
    );
}

extern char irq0_handler[];
extern char keyboard_isr[];
extern char syscall[];
extern char escalate_priv_isr[];

void load_idt(void) {
    /* Register all interrupts */
/*
    for (size_t i = 0; i < 256; i++)
        register_interrupt_handler(i, int_thunks[i], 0, 0x8e);
*/

    /* Exception handlers */
    for (size_t i = 0; i < 32; i++)
        register_interrupt_handler(i, 0x08, exception_thunks[i], 0x8e);

    register_interrupt_handler(0x20, 0x08, irq0_handler, 0x8e);
    register_interrupt_handler(0x21, 0x08, keyboard_isr, 0x8e);

    register_interrupt_handler(0x80, 0x19, syscall, 0xee);
    register_interrupt_handler(0x90, 0x08, escalate_priv_isr, 0xae);

    struct idt_ptr idt_ptr = {
        sizeof(idt) - 1,
        (uint64_t)idt
    };

    asm volatile (
        "lidt %0"
        :
        : "m" (idt_ptr)
        : "memory"
    );
}
