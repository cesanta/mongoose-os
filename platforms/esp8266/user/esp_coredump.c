#ifdef ESP_COREDUMP

#include <stdio.h>
#include <string.h>
#include <ets_sys.h>
#include <xtensa/corebits.h>
#include <stdint.h>
#include "esp_gdb.h"
#include "esp_exc.h"
#include "v7_esp_hw.h"
#include "esp_uart.h"
#include "esp_missing_includes.h"
#include "v7_esp.h"
#include "base64.h"

#define ESP_COREDUMP_UART_NO 1
#define ESP_COREDUMP_FILENO (ESP_COREDUMP_UART_NO + 1)

/* output an unsigned decimal integer */
static void uart_putdec(int fd, unsigned int n) {
  unsigned int tmp;
  unsigned long long p = 1;

  for (tmp = n; tmp > 0; tmp /= 10) {
    p *= 10;
  }
  p /= 10;
  if (p == 0) {
    p = 1;
  }

  for (; p > 0; p /= 10) {
    uart_putchar(fd, '0' + (unsigned int) (n / p) % 10);
  }
}

static int core_dump_emit_char_fd = 0;
static void core_dump_emit_char(char c) {
  uart_putchar(core_dump_emit_char_fd, c);
}

static void emit_core_dump_section(int fd, const char *name, uint32_t addr,
                                   uint32_t size) {
  uart_puts(fd, ",\"");
  uart_puts(fd, name);
  uart_puts(fd, "\": {\"addr\": ");
  uart_putdec(fd, addr);
  uart_puts(fd, ", \"data\": \"");
  core_dump_emit_char_fd = fd;
  cs_base64_encode2((unsigned char *) addr, size, core_dump_emit_char);
  uart_puts(fd, "\"}");
}

void esp_dump_core(int fd, struct regfile *regs) {
  if (fd == -1) {
    fd = ESP_COREDUMP_FILENO;
  }
  uart_puts(fd, "-------- Core Dump --------\n");

  uart_puts(fd, "{\"arch\": \"ESP8266\"");
  emit_core_dump_section(fd, "REGS", (uintptr_t) regs, sizeof(*regs));
  emit_core_dump_section(fd, "DRAM", 0x3FFE8000, 0x18000);
  emit_core_dump_section(fd, "ROM", 0x40000000, 0x10000);
  uart_puts(fd, "}\n");

  /*
   * IRAM and IROM can be obtained from the firmware/ dir.
   * We need the ELF binary anyway to do symbolic debugging anyway
   * so we can avoid sending here huge amount of data that's available
   * on the host where we run GDB.
   */

  uart_puts(fd, "-------- End Core Dump --------\n");
}

#endif /* ESP_COREDUMP */
