#include "mm/vm_defs.h"

OUTPUT_ARCH( "riscv" )

SECTIONS
{
  . = 0xffffffffc0000000;
  PROVIDE(rt_base = .);
  .text : {
    *(.text._start)
    *(.text.encl_trap_handler)
    *(.text)
  }
  . = ALIGN(RISCV_PAGE_SIZE);
  .rodata :
  {
    *(.rdata)
    *(.rodata)
  }
  .data : { *(.data) }
  .bss : { *(.bss) }
  . = ALIGN(RISCV_PAGE_SIZE);
  .kernel_stack : {
    . += 8 * RISCV_PAGE_SIZE;
    PROVIDE(kernel_stack_end = .);
  }

  /* Stack de usuario para el enclave */
  . = ALIGN(16);
  .stack (NOLOAD) : {
    _stack_start = .;
    . += 0x10000; /* 64KB de stack */
    _stack_end = .;
  }

  _end = .;
}
