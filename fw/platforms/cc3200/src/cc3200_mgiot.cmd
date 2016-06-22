/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Linker command file for the TI compiler.
 */

--retain=g_pfnVectors

#define RAM_BASE 0x20000000

/* System memory map */

MEMORY
{
    SRAM (RWX) : origin = 0x20000000, length = 0x40000
}

/* Section allocation in memory */

SECTIONS
{
    .intvecs:   > RAM_BASE
    GROUP {
      .init_array
      .vtable
      .text
      .const
      .data
      .bss
      .pinit
      .heap_start
    } > SRAM
    GROUP {
      .sysmem : type = DSECT
      .heap_end
      .stack
    } > SRAM(HIGH)
}

