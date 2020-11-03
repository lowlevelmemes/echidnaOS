#include <kernel.h>

__attribute__((noreturn))
void panic(struct gpr_state *regs, bool print_trace, const char *fmt, ...) {
    DISABLE_INTERRUPTS;

    va_list args;
    va_start(args, fmt);
    kprint(KPRN_PANIC, "Kernel panic:");
    kvprint(KPRN_PANIC, fmt, args);
    va_end(args);

    if (regs != NULL) {
        kprint(KPRN_PANIC, "CPU state at fault:");
        kprint(KPRN_PANIC, "  EAX: %8x  EBX: %8x  ECX: %8x  EDX: %8x",
                           regs->eax,
                           regs->ebx,
                           regs->ecx,
                           regs->edx);
        kprint(KPRN_PANIC, "  ESI: %8x  EDI: %8x  EBP: %8x  ESP: %8x",
                           regs->esi,
                           regs->edi,
                           regs->ebp,
                           regs->esp);
        kprint(KPRN_PANIC, "  EIP: %8x  EFLAGS: %8x", regs->eip, regs->eflags);
        kprint(KPRN_PANIC, "  CS: %4x SS: %4x DS: %4x ES: %4x FS: %4x GS: %4x",
                           regs->cs,
                           regs->ss,
                           regs->ds,
                           regs->es,
                           regs->fs,
                           regs->gs);
    }

    if (print_trace) {
        if (regs == NULL)
            print_stacktrace(KPRN_PANIC, NULL);
        else
            print_stacktrace(KPRN_PANIC, (size_t *)regs->ebp);
    }

    escalate_privilege();
    asm volatile (
        "1: hlt;"
        "jmp 1b;"
        ::: "memory"
    );
}
