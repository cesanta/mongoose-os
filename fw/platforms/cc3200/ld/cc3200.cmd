/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * Linker command file for the TI compiler.
 */

--retain=g_pfnVectors

/* External vars: APP_ORG SRAM_BASE_ADDR SRAM_SIZE */

/* System memory map */

MEMORY
{
    SRAM (RWX) : origin = APP_ORG, length = (SRAM_SIZE - (APP_ORG - SRAM_BASE_ADDR))
}

/* Section allocation in memory */

SECTIONS
{
    .intvecs:   > APP_ORG
    GROUP {
      .init_array
      .vtable
      .iram .iram.*
      .text .text.*
      .const
      .data
      .bss_start
      .bss
      .bss_end
      .pinit
      .heap_start
    } > SRAM
    GROUP {
      .sysmem : type = DSECT
      .heap_end
      .stack
    } > SRAM(HIGH)
}

