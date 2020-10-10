#include <kernel.h>
#include <stdint.h>

extern uint32_t TSS;
extern uint32_t TSS_size;

struct dt_entry gdt[9];

struct gdt_ptr {
    uint16_t size;
    uint32_t base;
} __attribute__((packed));

static struct gdt_ptr gdt_ptr = {
    (uint16_t)(sizeof(gdt) - 1),
    (uint32_t)gdt
};

void set_segment(struct dt_entry *dt, uint16_t entry, uint32_t base, uint32_t page_count) {
    dt[entry].base_low = (uint16_t)(base & 0x0000ffff);
    dt[entry].base_mid = (uint8_t)((base & 0x00ff0000) >> 16);
    dt[entry].base_high = (uint8_t)((base & 0xff000000) >> 24);

    dt[entry].limit_low = (uint16_t)((page_count - 1) & 0x0000ffff);
    dt[entry].granularity = (uint8_t)((dt[entry].granularity & 0b11110000) | ((page_count & 0x000f0000) >> 16));
}

void load_gdt(void) {
    // null pointer
    gdt[0].limit_low = 0;
    gdt[0].base_low = 0;
    gdt[0].base_mid = 0;
    gdt[0].access = 0;
    gdt[0].granularity = 0;
    gdt[0].base_high = 0;

    // define kernel code
    gdt[1].limit_low = 0xffff;
    gdt[1].base_low = 0x0000;
    gdt[1].base_mid = 0x00;
    gdt[1].access = 0b10011010;
    gdt[1].granularity = 0b11001111;
    gdt[1].base_high = 0x00;

    // define kernel data
    gdt[2].limit_low = 0xffff;
    gdt[2].base_low = 0x0000;
    gdt[2].base_mid = 0x00;
    gdt[2].access = 0b10010010;
    gdt[2].granularity = 0b11001111;
    gdt[2].base_high = 0x00;

    // define user code
    gdt[3].limit_low = 0x0000;
    gdt[3].base_low = 0x0000;
    gdt[3].base_mid = 0x00;
    gdt[3].access = 0b11111010;
    gdt[3].granularity = 0b11000000;
    gdt[3].base_high = 0x00;

    // define user data
    gdt[4].limit_low = 0x0000;
    gdt[4].base_low = 0x0000;
    gdt[4].base_mid = 0x00;
    gdt[4].access = 0b11110010;
    gdt[4].granularity = 0b11000000;
    gdt[4].base_high = 0x00;

    // define 16-bit code
    gdt[5].limit_low = 0xffff;
    gdt[5].base_low = 0x0000;
    gdt[5].base_mid = 0x00;
    gdt[5].access = 0b10011010;
    gdt[5].granularity = 0b10001111;
    gdt[5].base_high = 0x00;

    // define 16-bit data
    gdt[6].limit_low = 0xffff;
    gdt[6].base_low = 0x0000;
    gdt[6].base_mid = 0x00;
    gdt[6].access = 0b10010010;
    gdt[6].granularity = 0b10001111;
    gdt[6].base_high = 0x00;

    // define TSS segment
    gdt[7].limit_low = (uint16_t)(TSS_size & 0x0000ffff);
    gdt[7].base_low = (uint16_t)(TSS & 0x0000ffff);
    gdt[7].base_mid = (uint8_t)((TSS & 0x00ff0000) / 0x10000);
    gdt[7].access = 0b10001001;
    gdt[7].granularity = 0b00000000;
    gdt[7].base_high = (uint8_t)((TSS & 0xff000000) / 0x1000000);

    // define LDT segment
    gdt[8].limit_low = 0;
    gdt[8].base_low = 0;
    gdt[8].base_mid = 0;
    gdt[8].access = 0b10000010;
    gdt[8].granularity = 0;
    gdt[8].base_high = 0;

    // effectively load the gdt
    asm volatile (
        "lgdt %0;"
        "jmp 0x08:1f;"
        "1:"
        "mov ax, 0x10;"
        "mov ds, ax;"
        "mov es, ax;"
        "mov fs, ax;"
        "mov gs, ax;"
        "mov ss, ax;"
         :
         : "m" (gdt_ptr)
         : "eax", "memory"
    );

    asm volatile (
        "mov ax, 0x38;"
        "ltr ax;"
         :
         :
         : "eax", "memory"
    );
}

void load_ldt(uint32_t base, uint32_t entries) {
    uint32_t limit = entries ? entries * sizeof(struct dt_entry) - 1 : 0;

    gdt[8].base_low = (uint16_t)(base & 0x0000ffff);
    gdt[8].base_mid = (uint8_t)((base & 0x00ff0000) >> 16);
    gdt[8].base_high = (uint8_t)((base & 0xff000000) >> 24);

    gdt[8].limit_low = (uint16_t)(limit & 0x0000ffff);
    gdt[8].granularity = (uint8_t)((gdt[8].granularity & 0b11110000) | ((limit & 0x000f0000) >> 16));

    asm volatile (
        "mov ax, 0x40;"
        "lldt ax;"
         :
         :
         : "eax", "memory"
    );
}
