#include <stdint.h>
#include <kernel.h>

static const char *exception_names[] = {
    "Division by 0",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "???",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "???",
    "x87 exception",
    "Alignment check",
    "Machine check",
    "SIMD exception",
    "Virtualisation",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "Security"
};

void exception_handler(int exception_number, struct gpr_state *state, int has_error_code, uint32_t error_code) {
    tty_kputs("\n", 0);
    tty_kputs(exception_names[exception_number], 0);

    tty_kputs("\nException occurred at: ", 0);
    tty_kxtoa(state->cs, 0);
    text_putchar(':', 0);
    tty_kxtoa(state->eip, 0);

    if (has_error_code) {
        tty_kputs("\nError code: ", 0);
        tty_kxtoa(error_code, 0);
    }

    if (state->cs == 0x08) {
        tty_kputs("\nIn-kernel exception, system halted.", 0);
        for (;;)
            asm volatile ("hlt");
    }

    tty_kputs("\nTask terminated.\n", 0);
    task_quit(current_task, -1);
}
