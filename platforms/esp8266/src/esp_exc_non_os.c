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
#ifndef RTOS_SDK

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <user_interface.h>

#include <xtensa/corebits.h>

#include "mgos_mmap_esp_internal.h"

#include "esp_exc.h"
#include "esp_hw.h"
#include "esp_missing_includes.h"

/*
 * xtos low level exception handler (in rom)
 * populates an xtos_regs structure with (most) registers
 * present at the time of the exception and passes it to the
 * high-level handler.
 *
 * Note that the a1 (sp) register is clobbered (bug? necessity?),
 * however the original stack pointer can be inferred from the address
 * of the saved registers area, since the exception handler uses the same
 * user stack. This might be different in other execution modes on the
 * quite variegated xtensa platform family, but that's how it works on ESP8266.
 */

/*
 * xtruntime-frames defines this only for assembler.
 * It ends up being 256 bytes in ESP8266.
 */
#define ESF_TOTALSIZE 0x100

extern void system_restart_local_sdk(void);

NOINSTR void regs_from_xtos_frame(UserFrame *frame, struct regfile *regs) {
  regs->a[0] = frame->a0;
  regs->a[1] = (uint32_t) frame + ESF_TOTALSIZE;
  regs->a[2] = frame->a2;
  regs->a[3] = frame->a3;
  regs->a[4] = frame->a4;
  regs->a[5] = frame->a5;
  regs->a[6] = frame->a6;
  regs->a[7] = frame->a7;
  regs->a[8] = frame->a8;
  regs->a[9] = frame->a9;
  regs->a[10] = frame->a10;
  regs->a[11] = frame->a11;
  regs->a[12] = frame->a12;
  regs->a[13] = frame->a13;
  regs->a[14] = frame->a14;
  regs->a[15] = frame->a15;
  regs->pc = frame->pc;
  regs->sar = frame->sar;
  regs->litbase = RSR(LITBASE);
  regs->sr176 = 0;
  regs->sr208 = 0;
  regs->ps = frame->ps;
}

IRAM NOINSTR void esp_exception_handler(UserFrame *frame) {
  /* Not stored in the frame's fields for some reason. */
  uint32_t cause = RSR(EXCCAUSE);
  struct regfile regs;

#ifdef CS_MMAP
  uint32_t vaddr = RSR(EXCVADDR);
  int pc_inc = 0;

  if ((void *) vaddr >= MMAP_BASE) {
    if ((pc_inc = esp_mmap_exception_handler(vaddr, (uint8_t *) frame->pc,
                                             &frame->a2)) > 0) {
      frame->pc += pc_inc;
      return;
    }
  }
#endif

  regs_from_xtos_frame(frame, &regs);
  esp_exc_common(cause, &regs);
}

IRAM NOINSTR void esp_exc_from_xtos_int(uint32_t cause, uint32_t *sp) {
  /*
   * _xtos_l1int_handler has created a UserFrame structure on the stack.
   * Search for _xtos_l1int_handler first (return address on some function's
   * stack),
   * then zero in on UserFrame by searching for epc1, which will be the
   * PC value saved by _xtos_l1int_handler.
   */
  uint32_t epc1 = RSR(EPC1);
  /* 0x4000050c is the return address within _xtos_l1int_handler */
  while (sp < (uint32_t *) 0x40000000 && *sp != (uint32_t) 0x4000050c) sp++;
  while (sp < (uint32_t *) 0x40000000 && *sp != epc1) sp++;
  if (sp < (uint32_t *) 0x40000000) {
    struct regfile regs;
    regs_from_xtos_frame((UserFrame *) sp, &regs);
    esp_exc_common(cause, &regs);
  } else {
    esp_exc_printf("%x Not found!\r\n", epc1);
    abort();
  }
}

IRAM NOINSTR void esp_hw_wdt_isr(void *arg) {
  (void) arg;
  esp_exc_from_xtos_int(EXCCAUSE_HW_WDT, (uint32_t *) &arg);
}

NOINSTR void system_restart_local(void) {
  struct rst_info ri;
  system_rtc_mem_read(0, &ri, sizeof(ri));
  if (ri.reason == REASON_SOFT_WDT_RST) {
    /* So, here's how we got here:
     *  0) system_restart_local (that's us!) - 64 byte frame.
     *  1) pp_soft_wdt_feed_local - 32 bytes.
     *  2) static local function in the SDK, probably wDev_something - 16 bytes.
     *  3) wDev_ProcessFiq - 48 bytes.
     *  4) _xtos_l1int_handler - in ROM, ret 0x4000050c, no stack of its own
     *  5) XTOS user vector mode exception stack frame,
     *     aka struct UserFrame - 256 bytes (ESF_TOTALSIZE)
     *  6) ... pre-interrupt stack.
     *
     *  Since this originates in an interrupt, we can use that.
     */
    uint32_t *sp = (uint32_t *) &ri;
    //   while (sp < (uint32_t *) 0x40000000 && *sp != (uint32_t) 0x4000050c)
    //   sp++;
    esp_exc_from_xtos_int(EXCCAUSE_SW_WDT, sp);
  }
  system_restart_local_sdk();
}

NOINSTR void esp_exception_handler_init(void) {
  char causes[] = {EXCCAUSE_ILLEGAL,          EXCCAUSE_INSTR_ERROR,
                   EXCCAUSE_LOAD_STORE_ERROR, EXCCAUSE_DIVIDE_BY_ZERO,
                   EXCCAUSE_UNALIGNED,        EXCCAUSE_INSTR_PROHIBITED,
                   EXCCAUSE_LOAD_PROHIBITED,  EXCCAUSE_STORE_PROHIBITED};
  int i;
  for (i = 0; i < (int) sizeof(causes); i++) {
    _xtos_set_exception_handler(causes[i], esp_exception_handler);
  }
}

void esp_system_restart_low_level(void) {
  system_restart_local_sdk();
}

#endif /* !RTOS_SDK */
