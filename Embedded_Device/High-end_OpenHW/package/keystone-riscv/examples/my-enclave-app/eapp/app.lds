OUTPUT_ARCH( "riscv" )
 
/* Definir el punto de entrada */

ENTRY(eapp_entry)
 
/* Definir el tamaño de página */

PAGE_SIZE = 0x1000;
 
SECTIONS 

{

  /* Comenzar en una dirección alineada a página */

  . = 0x00001000;

  /* Sección de texto con el punto de entrada primero */

  .text : {

    *(.text.eapp_entry)  /* Asegurar que eapp_entry esté primero */

    *(.text._start)

    *(.text)

    *(.text.*)

  }

  /* Alinear a página y añadir sección de datos de solo lectura */

  . = ALIGN(PAGE_SIZE);

  .rodata : {

    *(.rdata)

    *(.rodata)

    *(.rodata.*)

  }

  /* Sección de datos */

  . = ALIGN(PAGE_SIZE);

  .data : {

    *(.data)

    *(.data.*)

  }

  /* Sección BSS */

  . = ALIGN(PAGE_SIZE);

  .bss : {

    *(.bss)

    *(.bss.*)

    *(COMMON)

  }

  /* Sección de depuración */

  .debug : {

    *(.debug)

    *(.debug.*)

  }

  /* Zona de malloc con alineación a página */

  . = ALIGN(PAGE_SIZE);

  .malloc_zone : {

    __malloc_start = .;

    /* Reservar 2MB para malloc */

    . = . + 0x200000;

    __malloc_zone_stop = .;

  }

  /* Marcar el final del programa */

  . = ALIGN(PAGE_SIZE);

  _end = .;

  /* Descartar secciones que podrían causar problemas */

  /DISCARD/ : {

    *(.eh_frame)

    *(.eh_frame_hdr)

    *(.eh_frame_entry)

    *(.eh_frame_entry.*)

    *(.interp)

    *(.note.*)

  }

}
 