/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 */

ENTRY(Reset_Handler)

_estack = SRAM_BASE_ADDR + SRAM_SIZE;
_init_stack_size = 0x400; /* stack size before FreeRTOS is running */

MEMORY {
  SRAM     (rwx) : ORIGIN = SRAM_BASE_ADDR,  LENGTH = SRAM_SIZE
  FLASH_S0 (rx)  : ORIGIN = FLASH_BASE_ADDR, LENGTH = FLASH_S0_SIZE
  FLASH_FS (r)   : ORIGIN = FLASH_BASE_ADDR + FLASH_S0_SIZE, LENGTH = FLASH_FS_SIZE
  FLASH_CS (rx)  : ORIGIN = FLASH_BASE_ADDR + FLASH_S0_SIZE + FLASH_FS_SIZE,
                   LENGTH = FLASH_SIZE - FLASH_FS_SIZE - FLASH_S0_SIZE
}

/* Define output sections */
SECTIONS {
  /* The startup code goes first into FLASH */
  .isr_vector : {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
  } > FLASH_S0

  /* Fill FS0 with 0xff to make initial mkfs faster (no erase). */
  .fs_img : {
    KEEP(*(.fs_img))
  } > FLASH_FS

  .text :
  {
    . = ALIGN(4);
    *(.text SORT(.text.*))
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } > FLASH_CS

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH_CS
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH_CS

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } > FLASH_CS
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } > FLASH_CS
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } > FLASH_CS

  .rodata : {
    *(.rodata SORT(.rodata.*))
  } > FLASH_CS

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data SORT(.data.*))
    *(.RamFunc SORT(.RamFunc.*))
    _edata = .;        /* define a global symbol at data end */
  } > SRAM AT > FLASH_CS

  /* Uninitialized data section */
  .bss : {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss SORT(.bss.*))
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } > SRAM

  /* For now, max size of no-cache region is 1K */
  .nocache : {
    . = ALIGN(NOCACHE_SIZE);
    __nocache_start__ = .;
    *(.nocache)
    . = ALIGN(NOCACHE_SIZE);
    __nocache_end__ = .;
  } > SRAM

  .heap : {
    . = ALIGN(8);
    _heap_start = .;
    . = . + (LENGTH(SRAM) - SIZEOF(.data) - SIZEOF(.bss) - SIZEOF(.nocache) - _init_stack_size);
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
