/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>
#include <freertos/xtensa_api.h>
#include <xtensa/corebits.h>

#include "common/platforms/esp/src/esp_mmap.h"

extern void xt_unhandled_exception(XtExcFrame *frame);

void esp32_exception_handler(XtExcFrame *frame) {
  if ((void *) frame->excvaddr >= MMAP_BASE) {
    int off = esp_mmap_exception_handler(frame->excvaddr, (uint8_t *) frame->pc,
                                         (long *) &frame->a2);
    if (off > 0) {
      frame->pc += off;
      return;
    }
  }

  xt_unhandled_exception(frame);
}

NOINSTR void esp32_exception_handler_init(void) {
  xt_set_exception_handler(EXCCAUSE_LOAD_PROHIBITED, esp32_exception_handler);
}
