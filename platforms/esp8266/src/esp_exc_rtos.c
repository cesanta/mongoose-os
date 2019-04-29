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
#ifdef RTOS_SDK

#include <stdint.h>
#include <stdlib.h>

#include <esp_common.h>

#include <xtensa/corebits.h>

#include "common/platforms/esp/src/esp_mmap.h"
#include "esp_exc.h"
#include "esp_hw.h"

struct exc_frame {
  uint32_t exit; /* exit pointer for dispatch, points to _xt_user_exit */
  uint32_t pc;
  uint32_t ps;
  uint32_t a[16];
  uint32_t sar;
};

void _xt_user_exit(void);

IRAM NOINSTR void __wrap_user_fatal_exception_handler(uint32_t cause) {
/*
 * Note that we don't get here with SYSTEM_CALL or LOAD_STORE_ERROR.
 * SDK has partially filled an exc_frame struct for us that has been placed
 * on the stack of the calling function. a14 and a15 have not bee saved, save
 * them now.
 */
#ifdef CS_MMAP
  uint32_t vaddr = RSR(EXCVADDR);
#endif

  uint32_t a14, a15;
  __asm volatile(
      "mov.n %0, a14\n"
      "mov.n %1, a15"
      : "=a"(a14), "=a"(a15));

  /* We identify the frame by searching for the exit pointer (_xt_user_exit). */
  uint32_t *sp = &cause;
  while (*sp != (uint32_t) _xt_user_exit) sp++;
  struct exc_frame *f = (struct exc_frame *) sp;

#ifdef CS_MMAP
  if (cause == EXCCAUSE_LOAD_PROHIBITED && (void *) vaddr >= MMAP_BASE) {
    int pc_inc =
        esp_mmap_exception_handler(vaddr, (uint8_t *) f->pc, (long *) &f->a[2]);
    if (pc_inc > 0) {
      f->pc += pc_inc;

/*
 * TODO(dfrank): at the moment it doesn't work on RTOS, and the failure is very
 * weird: without the commented snippet below, WDT gets triggered. But if we
 * spend some time here (e.g. busyloop as below), then it works.
 *
 * Need to figure what's going on, and fix.
 */
#if 0
      {
        volatile int i = 0;
        for (i = 0; i < 100000; i++) {
        }
        mgos_wdt_feed();
      }
#endif
      return;
    }
  }
#endif

  /* Now convert it to GDB frame. */
  struct regfile regs;
  memcpy(regs.a, f->a, sizeof(regs.a));
  regs.pc = f->pc;
  regs.sar = f->sar;
  regs.ps = f->ps;
  regs.a[1] = ((uint32_t) sp) + sizeof(*f);
  regs.a[14] = a14;
  regs.a[15] = a15;
  regs.litbase = RSR(LITBASE);
  regs.sr176 = 0;
  regs.sr208 = 0;
  esp_exc_common(cause, &regs);
}

IRAM NOINSTR void esp_hw_wdt_isr(void *arg) {
  (void) arg;
  /* ISR has the same exception frame on the stack. */
  __wrap_user_fatal_exception_handler(EXCCAUSE_HW_WDT);
}

/* _OurNMIExceptionHandler has stored registers for us, and in what looks like
 * struct regfile too - how convenient! */
IRAM NOINSTR void __wrap_ShowCritical(void) {
  struct regfile regs;
  memcpy(&regs, &g_exc_regs, sizeof(regs));
  esp_exc_common(EXCCAUSE_SW_WDT, &regs);
}

extern void system_restart_in_nmi(void);

void esp_system_restart_low_level(void) {
  system_restart_in_nmi();
}

#endif /* RTOS_SDK */
