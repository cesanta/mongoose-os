/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_EXC_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_EXC_H_

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

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_EXC_H_ */
