#include <kernel.h>
#include <stdint.h>
#include <cio.h>

void map_PIC(uint8_t PIC0Offset, uint8_t PIC1Offset) {
	uint8_t mask0;
	uint8_t mask1;

	mask0 = get_PIC0_mask();		// save PIC 0 and 1's masks
	mask1 = get_PIC1_mask();

	port_out_b(0x20, 0x11);			// initialise
	port_out_b(0xA0, 0x11);

	port_out_b(0x21, PIC0Offset);	// set offsets
	port_out_b(0xA1, PIC1Offset);

	port_out_b(0x21, 0x04);			// PIC wiring info
	port_out_b(0xA1, 0x02);

	port_out_b(0x21, 0x01);			// environment info
	port_out_b(0xA1, 0x01);

	set_PIC0_mask(mask0);		// restore masks
	set_PIC1_mask(mask1);
}

void set_PIC0_mask(uint8_t mask) {
    port_out_b(0x21, mask);
    return;
}

void set_PIC1_mask(uint8_t mask) {
    port_out_b(0xA1, mask);
    return;
}

uint8_t get_PIC0_mask(void) {
    return port_in_b(0x21);
}

uint8_t get_PIC1_mask(void) {
    return port_in_b(0xA1);
}

int pic_io_wrapper(uint32_t dev, uint64_t loc, int type, uint8_t payload) {
	switch (type) {
		case DF_GET_MASK:
			if (dev) {
				return get_PIC1_mask();
			} else {
				return get_PIC0_mask();
			}

		case DF_SET_MASK:
			if (dev) {
				set_PIC1_mask(payload);
			} else {
				set_PIC0_mask(payload);
			}
			return 0;
	}
}

void init_pic(void) {
	kernel_add_device("I8259 Master", 0, 0, &pic_io_wrapper);
	kernel_add_device("I8259 Slave", 1, 0, &pic_io_wrapper);
}
