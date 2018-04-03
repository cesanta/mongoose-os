/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
