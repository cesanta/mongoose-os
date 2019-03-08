/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
 * All rights reserved
 */

ENTRY(rs14100_entry)

/* Initial stack location and size (before RTOS). */
_stack = SRAM_BASE_ADDR + SRAM_SIZE;
_stack_size = 0x400;

MEMORY {
  FLASH (rx)  : ORIGIN = FLASH_BASE_ADDR, LENGTH = FLASH_SIZE
  SRAM  (rwx) : ORIGIN = SRAM_BASE_ADDR,  LENGTH = SRAM_SIZE
}

/* Define output sections */
SECTIONS {
  .flash_int_vectors : {
    . = ALIGN(256);  /* Vectors must be aligned to 0x100 */
    KEEP(*(.flash_int_vectors))
  } > FLASH

  .text : {
    . = ALIGN(4);
    *(.text SORT(.text.*))
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
  } > FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } > FLASH
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } > FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } > FLASH

  .rodata : {
    *(.rodata SORT(.rodata.*))
  } > FLASH

  .data : {
    _data_start = .;
    . = ALIGN(256);  /* Vectors must be aligned to 0x100 */
    KEEP(*(.ram_int_vectors))
    *(.data SORT(.data.*))
    *(.RamFunc SORT(.RamFunc.*))
    _data_end = .;
  } > SRAM AT > FLASH
  _data_flash_start = LOADADDR(.data);

  .bss : {
    _bss_start = .;
    *(.bss SORT(.bss.*))
    *(COMMON)
    . = ALIGN(4);
    _bss_end = .;
  } > SRAM

  .heap : {
    . = ALIGN(8);
    _heap_start = .;
    . = . + (LENGTH(SRAM) - SIZEOF(.data) - SIZEOF(.bss) - _stack_size);
    . = ALIGN(8);
    _heap_end = .;
  } > SRAM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
