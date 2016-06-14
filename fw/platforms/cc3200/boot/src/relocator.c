/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * ROM loader loads and executes program at 0x20004000.
 * This shim moves code that comes right after it to _text_start (0x20000000).
 */

#include <inttypes.h>

extern uint32_t _reloc_end;  /* Symbol at the end of myreloc section. */
extern uint32_t _text_start; /* Destination address to copy to. */
extern uint32_t _bss_start;  /* Symbol at the end of area to copy */

__attribute__((section(".myreloc"))) void relocate(void) {
  uint32_t *from = &_reloc_end;
  uint32_t *to = &_text_start;
  uint32_t len = (&_bss_start - &_text_start);
  while (len-- > 0) *to++ = *from++;
  __asm(
      "ldr r0, =_reloc_end\n"
      "ldr sp, [r0]\n"
      "add r0, r0, #4\n"
      "ldr r1, [r0]\n"
      "bx  r1");
}

__attribute__((section(".myrelocints"))) void (*const reloc_int_vecs[2])(
    void) = {
    0,        /* We don't need stack */
    relocate, /* Our entry point. */
};
