/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
 * All rights reserved
 */

ENTRY(rs14100_entry)

/* Initial stack location and size (before RTOS). */
_stack = SRAM_BASE + SRAM_SIZE;
_stack_size = 0x400;

MEMORY {
  FLASH (rx)  : ORIGIN = FLASH_BASE + FLASH_APP_OFFSET, LENGTH = FLASH_APP_SIZE
  SRAM  (rwx) : ORIGIN = SRAM_BASE, LENGTH = SRAM_SIZE
}

/* Define output sections */
SECTIONS {
  .flash_int_vectors : {
    KEEP(*(.flash_int_vectors))
  } > FLASH

  /* Small area at the top of RAM for communicating between loader and app. */
  .boot_state : {
    *(.boot_state)
  } > SRAM

  .data : {
    _data_start = .;
    KEEP(*(.ram_int_vectors))
    *(.data SORT(.data.*))
    *(SORT(.rodata.IRAM.*))
    *(SORT(.text.IRAM.*))
    _data_end = .;
  } > SRAM AT > FLASH
  _data_flash_start = LOADADDR(.data);

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

  .bss : {
    _bss_start = .;
    *(.bss SORT(.bss.*))
    *(COMMON)
    . = ALIGN(4);
    _bss_end = .;
  } > SRAM

  .heap : {
    _heap_start = .;
    . = . + (LENGTH(SRAM) - SIZEOF(.data) - SIZEOF(.bss) - _stack_size - 1024 - 8);
    _heap_end = .;
  } > SRAM

  .stack : {
    . = ALIGN(4);
    . = . + _stack_size;
    _stack = .;
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
