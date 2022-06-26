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

#include <freertos/xtensa_api.h>
#include <stdint.h>
#include <xtensa/corebits.h>

#include "mgos_mmap_esp_internal.h"

extern void xt_unhandled_exception(XtExcFrame *frame);

void esp32_exception_handler(XtExcFrame *frame) {
#ifdef CS_MMAP
  if ((void *) frame->excvaddr >= MMAP_BASE) {
    int off = esp_mmap_exception_handler(frame->excvaddr, (uint8_t *) frame->pc,
                                         (long *) &frame->a2);
    if (off > 0) {
      frame->pc += off;
      return;
    }
  }
#endif
  xt_unhandled_exception(frame);
}

NOINSTR void esp32_exception_handler_init(void) {
  xt_set_exception_handler(EXCCAUSE_LOAD_PROHIBITED, esp32_exception_handler);
}
