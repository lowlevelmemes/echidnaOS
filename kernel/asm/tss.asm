extern ring0_stack.top
extern ring1_stack.top

global TSS
global TSS_size

global syscall_stack

section .data

align 4
TSS_begin:
    dd 0
    dd ring0_stack.top
    dd 0x10
    syscall_stack: dd ring1_stack.top
    dd 0x21
    times 21 dd 0
TSS_end:

TSS dd TSS_begin
TSS_size dd TSS_end - TSS_begin - 1
