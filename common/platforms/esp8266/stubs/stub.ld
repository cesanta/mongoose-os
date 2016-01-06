/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

MEMORY {
  iram : org = 0x40100000, len = 0x8000
  dram : org = 0x3FFE8000, len = 0x14000
}

ENTRY(stub_main)

SECTIONS {
  .params 0x40100000 : {
    _params_start = ABSOLUTE(.);
    *(.params)
    _params_end = ABSOLUTE(.);
  } > iram

  .code : ALIGN(4) {
    _code_start = ABSOLUTE(.);
    *(.literal)
    *(.text .text.*)
  } > iram

  .data : {
    _data_start = ABSOLUTE(.);
    *(.bss .data)
    *(.rodata .rodata.*)
  } > dram
}

INCLUDE "eagle.rom.addr.v6.ld"

PROVIDE(SPIFlashModeConfig = 0x40004568);
