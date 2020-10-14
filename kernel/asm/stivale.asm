section .bss

global ring0_stack
global ring1_stack
global ring0_stack.top
global ring1_stack.top

ring0_stack:
    resb 32768
  .top:

ring1_stack:
    resb 32768
  .top:

section .stivalehdr

header:
    .stack dq ring1_stack.top
    .flags dw 0
    times 3 dw 0
    dq 0
