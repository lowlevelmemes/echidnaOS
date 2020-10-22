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
    kprint(KPRN_WARN, "%s occurred at: %x:%x", exception_names[exception_number],
           state->cs, state->eip);

    if (has_error_code)
        kprint(KPRN_WARN, "Error code: %x", error_code);

    if (state->cs == 0x08 || state->cs == 0x19) {
        switch_tty(0);
        panic(state, false, "In-kernel exception, system halted.");
    }

    kprint(KPRN_WARN, "Last syscall: %x", last_syscall);

    kprint(KPRN_WARN, "Task terminated.");
    task_quit(current_task, -1);
}
