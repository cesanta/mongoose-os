#ifdef ESP_GDB_SERVER

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

#ifdef RTOS_SDK
void system_soft_wdt_feed();
#endif

#define ESP_GDB_UART_NO 0
#define ESP_GDB_FILENO (ESP_GDB_UART_NO + 1)

/* TODO(mkm): not sure if gdb guarantees lowercase hex digits */
#define fromhex(c) \
  (((c) &0x40) ? ((c) &0x20 ? (c) - 'a' + 10 : (c) - 'A' + 10) : (c) - '0')
#define hexdigit(n) (((n) < 10) ? '0' + (n) : 'a' + ((n) -10))

static uint8_t gdb_send_checksum;

void gdb_nack() {
  uart_puts(ESP_GDB_FILENO, "-");
}

void gdb_ack() {
  uart_puts(ESP_GDB_FILENO, "+");
}

void gdb_begin_packet() {
  uart_puts(ESP_GDB_FILENO, "$");
  gdb_send_checksum = 0;
}

void gdb_end_packet() {
  uart_putchar(ESP_GDB_FILENO, '#');
  uart_putchar(ESP_GDB_FILENO, hexdigit(gdb_send_checksum >> 4));
  uart_putchar(ESP_GDB_FILENO, hexdigit(gdb_send_checksum & 0xF));
}

void gdb_putchar(char ch) {
  gdb_send_checksum += (uint8_t) ch;
  uart_putchar(ESP_GDB_FILENO, ch);
}

/* output a string while computing the checksum */
void gdb_putstr(char *str) {
  while (*str) gdb_putchar(*str++);
}

void gdb_putbyte(uint8_t val) {
  gdb_putchar(hexdigit(val >> 4));
  gdb_putchar(hexdigit(val & 0xF));
}

/* 32-bit integer in native byte order */
void gdb_putint(uint32_t val) {
  int i;
  uint8_t *v = (uint8_t *) &val;
  for (i = 0; i < 4; i++) {
    gdb_putbyte(v[i]);
  }
}

/* send a gdb packet with checksum */
void gdb_send_packet(char *str) {
  gdb_begin_packet();
  gdb_putstr(str);
  gdb_end_packet();
}

uint8_t gdb_read_unaligned(uint8_t *addr) {
  if (addr < (uint8_t *) ESP_LOWER_VALID_ADDRESS ||
      addr >= (uint8_t *) ESP_UPPER_VALID_ADDRESS) {
    return 0;
  }

  return read_unaligned_byte(addr);
}

/*
 * Handles the GDB server protocol.
 * We currently support only the simple command set.
 *
 * Data is exchanged in packets like `$Cxxxxx#cc`
 * where `C` is a single letter command name, `xxxx` is some data payload,
 * and `cc` is a two digit hex checksum of the packet body.
 * Replies follow the same structure except that they lack the command symbol.
 *
 * For a more complete description of the protocol, see
 * https://sourceware.org/gdb/current/onlinedocs/gdb/Remote-Protocol.html
 */
void gdb_handle_char(struct regfile *regs, int ch) {
  static enum {
    GDB_JUNK,
    GDB_DATA,
    GDB_CHECKSUM,
    GDB_CHECKSUM2
  } state = GDB_JUNK;
  static char data[128];
  static int pos = 0;
  static uint8_t checksum;

  switch (state) {
    case GDB_JUNK:
      if (ch == '$') {
        checksum = 0;
        state = GDB_DATA;
      }
      break;
    case GDB_DATA:
      if (ch == '#') {
        state = GDB_CHECKSUM;
        break;
      }
      /* ignore too long commands, by acking and sending empty response */
      if (pos > sizeof(data)) {
        state = GDB_JUNK;
        gdb_ack();
        gdb_send_packet("");
        break;
      }
      checksum += (uint8_t) ch;
      data[pos++] = ch;
      break;
    case GDB_CHECKSUM:
      if (fromhex(ch) != (checksum >> 4)) {
        gdb_nack();
        state = GDB_JUNK;
      } else {
        state = GDB_CHECKSUM2;
      }
      break;
    case GDB_CHECKSUM2:
      state = GDB_JUNK;
      if (fromhex(ch) != (checksum & 0xF)) {
        gdb_nack();
        pos = 0;
        break;
      }
      gdb_ack();

      /* process commands */
      switch (data[0]) {
        case '?':
          /* stop status */
          gdb_send_packet("S09"); /* TRAP */
          break;
        case 'm': {
          /* read memory */
          int i;
          uint32_t addr = 0;
          uint32_t num = 0;
          for (i = 1; i < pos && data[i] != ','; i++) {
            addr <<= 4;
            addr |= fromhex(data[i]);
          }
          for (i++; i < pos; i++) {
            num <<= 4;
            num |= fromhex(data[i]); /* should be decimal */
          }
          gdb_begin_packet();
          for (i = 0; i < num; i++) {
            gdb_putbyte(gdb_read_unaligned(((uint8_t *) addr) + i));
          }
          gdb_end_packet();
          break;
        }
        case 'g': {
          /* dump registers */
          int i;
          gdb_begin_packet();
          for (i = 0; i < sizeof(*regs); i++) {
            gdb_putbyte(((uint8_t *) regs)[i]);
          }
          gdb_end_packet();
          break;
        }
        default:
          gdb_send_packet("");
          break;
      }
      pos = 0;
      break;
  }
}

/* The user should detach and let gdb do the talkin' */
void gdb_server(struct regfile *regs) {
  uart_puts(ESP_GDB_FILENO, "Waiting for gdb\n");
  /*
   * polling since we cannot wait for interrupts inside
   * an interrupt handler of unknown level.
   *
   * Interrupts disabled so that the user (or v7 prompt)
   * uart interrupt handler doesn't interfere.
   */

  for (;;) {
#ifdef RTOS_SDK
    system_soft_wdt_feed();
#endif
    int ch = blocking_read_uart();
    if (ch != -1) gdb_handle_char(regs, ch);
  }
}

#endif /* ESP_GDB_SERVER */
