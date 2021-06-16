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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <xtensa/xtruntime-frames.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Register file in the format lx106 gdb port expects it.
 *
 * Inspired by gdb/regformats/reg-xtensa.dat from
 * https://github.com/jcmvbkbc/crosstool-NG/blob/lx106-g%2B%2B/overlays/xtensa_lx106.tar
 */
struct regfile {
  uint32_t a[16];
  uint32_t pc;
  uint32_t sar;
  uint32_t litbase;
  uint32_t sr176;
  uint32_t sr208;
  uint32_t ps;
};

/* Special "cause" values. */
#define EXCCAUSE_ABORT 100
#define EXCCAUSE_SW_WDT 101
#define EXCCAUSE_HW_WDT 102

void esp_exception_handler(UserFrame *frame);
void esp_exception_handler_init(void);
void esp_print_reset_info(void);
void esp_print_exc_info(uint32_t cause, struct regfile *regs);
void esp_exc_putc(int c);
void esp_exc_puts(const char *s);
void esp_exc_printf(const char *fmt, ...); /* Note: uses 100-byte buffer */

void esp_exc_common(uint32_t cause, struct regfile *regs);
/*
 * NMI exception handler will store registers in this struct.
 */
extern struct regfile g_exc_regs;

void esp_exc_extract_backtrace(void *sp, char *buf, int buf_len);

extern uint32_t ets_task_top_of_stack;
/*
 * Tasks share a 4K stack. Detect overflow as soon as we can.
 * This is a big problem because on other platforms we have 8K stack.
 * TODO(rojer): Try relocating stack and extending it, reuse 4K region for heap.
 */
#define MGOS_STACK_CANARY_VAL 0x777
#define MGOS_STACK_CANARY_LOC (&ets_task_top_of_stack)

void esp_report_stack_overflow(int tag1, int tag2, void *tag3);

extern uint32_t esp_stack_canary_en;

static inline __attribute__((always_inline)) bool esp_check_stack_overflow(
    int tag1, int tag2, void *tag3) {
  uint32_t en = esp_stack_canary_en;
  if ((*MGOS_STACK_CANARY_LOC & en) == (MGOS_STACK_CANARY_VAL & en)) {
    return false;
  }
  *MGOS_STACK_CANARY_LOC = MGOS_STACK_CANARY_VAL;
  esp_report_stack_overflow(tag1, tag2, tag3);
  return true;
}

#ifdef __cplusplus
}
#endif
