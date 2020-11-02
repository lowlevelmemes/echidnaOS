extern ring0_stack.top

global TSS
global TSS_size

section .data

align 4
TSS_begin:
    dd 0
    dd ring0_stack.top
    dd 0x10
    dd 0xf000
    dd 0x21
    times 21 dd 0
TSS_end:

TSS dd TSS_begin
TSS_size dd TSS_end - TSS_begin - 1
