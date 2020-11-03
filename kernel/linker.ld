OUTPUT_FORMAT(elf32-i386)
ENTRY(kernel_init)

SECTIONS
{
  . = 0x100000;

  .stivalehdr : { *(.stivalehdr) }
  .text       : { *(.text*)   }
  .rodata     : { *(.rodata*) }
  .data       : { *(.data*)   }
  .bss        : { *(.bss*) *(COMMON) }
  .symlist    : { *(.symlist*) }
}
