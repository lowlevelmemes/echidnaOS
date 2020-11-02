section .bss

global ring0_stack
global ring0_stack.top

ring0_stack:
    resb 32768
  .top:

section .stivalehdr

header:
    .stack dq 0xf000
    .flags dw 0
    times 3 dw 0
    dq 0
