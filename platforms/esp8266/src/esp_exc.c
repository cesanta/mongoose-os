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

#include "esp_exc.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef RTOS_SDK
#include <esp_common.h>
#else
#include <user_interface.h>
#endif

#include "common/cs_base64.h"
#include "common/cs_dbg.h"
#include "esp_missing_includes.h"

#include "mgos_core_dump.h"
#include "mgos_debug.h"
#include "mgos_hal.h"
#include "mgos_vfs.h"

#include "esp_coredump.h"
#include "esp_hw.h"
#include "esp_hw_wdt.h"
#include "esp_uart.h"

extern void esp_system_restart_low_level(void);

bool s_rebooting = false;
struct regfile g_exc_regs;

void esp_exc_putc(int c) {
  int uart_no = mgos_get_stderr_uart();
  if (uart_no < 0) return;
  while (esp_uart_tx_fifo_len(uart_no) > 125) {
  }
  esp_uart_tx_byte(uart_no, c);
}

void esp_exc_puts(const char *s) {
  while (*s != '\0') {
    esp_exc_putc(*s++);
  }
}

void esp_exc_printf(const char *fmt, ...) {
  char buf[100];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  esp_exc_puts(buf);
}

extern uint32_t ets_task_top_of_stack;

void esp_exc_extract_backtrace(void *sp, char *buf, int buf_len) {
  // Try to find some code pointers in the overflowed and top parts of stack.
  for (const uint32_t *p = sp; p < (uint32_t *) 0x40000000; p++) {
    if (((uintptr_t) p & 0xf) != 0xc) continue;
    uint32_t v = *p;
    /* Does it look like an IRAM or flash pointer? */
    if (!((v >= 0x40100000 && v < 0x40108000) ||
          (v >= 0x40200000 && v < 0x40300000))) {
      continue;
    }
    /* Does it look like a return pointer?
     * Check instruction immediately before the one that the pointer points to,
     * it should be a call instruction. */
    uint32_t vp = *((uint32_t *) (v - 3));
    bool is_call =
        ((vp & 0x3f) == 5 /* call0 */ || (vp & 0xffffff) == 0xc0 /* callx0 */);
    if (!is_call) continue;
    int l = snprintf(buf, buf_len - 1, "%s %d:%#08lx", buf,
                     (((intptr_t) &ets_task_top_of_stack) - (intptr_t) p), v);
    if (l >= buf_len - 10) break;
  }
}

void esp_print_exc_info(uint32_t cause, struct regfile *regs) {
  switch (cause) {
    case EXCCAUSE_ABORT:
      esp_exc_printf("\r\nabort() @ 0x%08x\r\n", regs->a[0]);
      break;
    case EXCCAUSE_SW_WDT:
    case EXCCAUSE_HW_WDT:
      esp_exc_printf("\r\n%cW WDT @ 0x%08x\r\n",
                     (cause == EXCCAUSE_SW_WDT ? 'S' : 'H'), regs->pc);
      break;
    default:
      esp_exc_printf("\r\nException %u @ 0x%08x, vaddr 0x%08x\r\n", cause,
                     regs->pc, RSR(EXCVADDR));
      break;
  }
  esp_exc_printf(" A0: 0x%08x  A1: 0x%08x  A2: 0x%08x  A3: 0x%08x\r\n",
                 regs->a[0], regs->a[1], regs->a[2], regs->a[3]);
  esp_exc_printf(" A4: 0x%08x  A5: 0x%08x  A6: 0x%08x  A7: 0x%08x\r\n",
                 regs->a[4], regs->a[5], regs->a[6], regs->a[7]);
  esp_exc_printf(" A8: 0x%08x  A9: 0x%08x A10: 0x%08x A11: 0x%08x\r\n",
                 regs->a[8], regs->a[9], regs->a[10], regs->a[11]);
  esp_exc_printf("A12: 0x%08x A13: 0x%08x A14: 0x%08x A15: 0x%08x\r\n",
                 regs->a[12], regs->a[13], regs->a[14], regs->a[15]);
  esp_exc_printf("\r\n(exc SP: %p)\r\n", &cause);
}

IRAM NOINSTR void dummy_putc(char c) {
  (void) c;
}

NOINSTR __attribute__((noreturn)) void esp_exc_common(uint32_t cause,
                                                      struct regfile *regs) {
  /* Disable interrupts. This takes care of SW WDT and other activity. */
  mgos_ints_disable();
  os_install_putc1(dummy_putc);
  /* Set up 1-stage HW WDT to give us a kick if we are not done in ~27 secs. */
  esp_hw_wdt_setup(ESP_HW_WDT_26_8_SEC, ESP_HW_WDT_DISABLE);
  esp_hw_wdt_enable();
  if (s_rebooting) {
    /* We are rebooting anyway, don't raise any noise. */
    esp_system_restart_low_level();
  }
  esp_print_exc_info(cause, regs);
  esp_dump_core(cause, regs);
#ifdef MGOS_STOP_ON_EXCEPTION
  esp_exc_puts("NOT rebooting\r\n");
  esp_hw_wdt_disable();
#else
  esp_exc_puts("rebooting\r\n");
  esp_system_restart_low_level();
#endif /* MGOS_STOP_ON_EXCEPTION */
  while (1) {
  }
}

IRAM NOINSTR void abort(void) {
  /*
   * We construct the register frame ourselves instead of causing an exception
   * because we want abort() to work even when interrupts are disabled (e.g. in
   * critical sections).
   */
  struct regfile regs;
  struct regfile *rp = &regs;
  __asm volatile(
      "\
      s32i   a0,  a1, 0     \n\
      s32i   a1,  a1, 4     \n\
      s32i   a2,  a1, 8     \n\
      s32i   a3,  a1, 12    \n\
      s32i   a4,  a1, 16    \n\
      s32i   a5,  a1, 20    \n\
      s32i   a6,  a1, 24    \n\
      s32i   a7,  a1, 28    \n\
      s32i   a8,  a1, 32    \n\
      s32i   a9,  a1, 36    \n\
      s32i   a10, a1, 40    \n\
      s32i   a11, a1, 44    \n\
      s32i   a12, a1, 48    \n\
      s32i   a13, a1, 52    \n\
      s32i   a14, a1, 56    \n\
      s32i   a15, a1, 60    \n\
      movi   a2,  abort     \n\
      addi   a2,  a2, 38    \n\
      s32i   a2,  a1, 64    \n\
      rsr    a2,  sar       \n\
      s32i   a2,  a1, 68    \n\
      rsr    a2,  litbase   \n\
      s32i   a2,  a1, 72    \n\
      rsr    a2,  ps        \n\
      s32i   a2,  a1, 84    \n\
  "
      : "=a"(rp)
      :
      : "a2");
  esp_exc_common(EXCCAUSE_ABORT, &regs);
  /* esp_exc_common does not return. */
}

void mgos_dev_system_restart(void) {
  esp_system_restart_low_level();
  // Not reached
  while (1) {
  }
}

void esp_print_reset_info(void) {
  struct rst_info *ri = system_get_rst_info();
  const char *reason_str;
  int print_exc_info = 0;
  switch (ri->reason) {
    case REASON_DEFAULT_RST:
      reason_str = "power on";
      break;
    case REASON_WDT_RST:
      reason_str = "HW WDT";
      print_exc_info = 1;
      break;
    case REASON_EXCEPTION_RST:
      reason_str = "exception";
      print_exc_info = 1;
      break;
    case REASON_SOFT_WDT_RST:
      reason_str = "SW WDT";
      print_exc_info = 1;
      break;
    case REASON_SOFT_RESTART:
      reason_str = "soft reset";
      break;
    case REASON_DEEP_SLEEP_AWAKE:
      reason_str = "deep sleep wake";
      break;
    case REASON_EXT_SYS_RST:
      reason_str = "sys reset";
      break;
    default:
      reason_str = "???";
      break;
  }
  LOG(LL_INFO, ("Reset cause: %lu (%s)", ri->reason, reason_str));
  if (print_exc_info) {
    LOG(LL_INFO,
        ("Exc info: cause=%lu epc1=0x%08lx epc2=0x%08lx epc3=0x%08lx "
         "vaddr=0x%08lx depc=0x%08lx",
         ri->exccause, ri->epc1, ri->epc2, ri->epc3, ri->excvaddr, ri->depc));
  }
}
