/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

MEMORY {
  SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 0x40000
}

SECTIONS {
  .relocator 0x20004000 : {
    KEEP(*(.myrelocints))
    . = ALIGN (4);
    KEEP(*(.myreloc))
    . = ALIGN(4);
    _reloc_end = .;
  }

  .text 0x20000000 + (LENGTH(SRAM) - 0x4000) : {
    _text_start = .;
    KEEP(*(.intvecs))
    *(.text*)
    *(.rodata*)
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(8);
  } > SRAM

  .data : {
    *(.data*)
    . = ALIGN (8);
  } > SRAM

  .bss : {
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    _bss_end = .;
  } > SRAM

}
