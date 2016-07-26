/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <inttypes.h>
#include <stdlib.h>

/* Driverlib includes */
#include "hw_types.h"

#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "pin.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"
#include "utils.h"

/*
 * We want to disable SL_INC_STD_BSD_API_NAMING, so we include user.h ourselves
 * and undef it.
 */
#define PROVISIONING_API_H_
#include <simplelink/user.h>
#undef PROVISIONING_API_H_
#undef SL_INC_STD_BSD_API_NAMING

#undef __CONCAT
#include <simplelink/include/simplelink.h>

#include "device.h"
#include "fs.h"

#include "boot.h"

#define SYS_CLK 80000000
#define CONSOLE_BAUD_RATE 115200
#define CONSOLE_UART UARTA0_BASE
#define CONSOLE_UART_INT INT_UARTA0
#define CONSOLE_UART_PERIPH PRCM_UARTA0

/* Int vector table, defined in startup_gcc.c */
extern void (*const int_vectors[])(void);

void abort() {
  MAP_UARTCharPut(CONSOLE_UART, 'X');
  while (1) {
  }
}

void uart_puts(const char *s) {
  for (; *s != '\0'; s++) {
    MAP_UARTCharPut(CONSOLE_UART, *s);
  }
}

static void print_addr(uint32_t addr) {
  char buf[10];
  __utoa(addr, buf, 16);
  uart_puts(buf);
}

int load_image(const char *fn, _u8 *dst) {
  _i32 fh;
  SlFsFileInfo_t fi;
  _i32 r = sl_FsGetInfo((const _u8 *) fn, 0, &fi);
  MAP_UARTCharPut(CONSOLE_UART, (r == 0 ? '+' : '-'));
  if (r != 0) return r;
  {
    char buf[20];
    __utoa(fi.FileLen, buf, 10);
    uart_puts(buf);
  }
  r = sl_FsOpen((const _u8 *) fn, FS_MODE_OPEN_READ, NULL, &fh);
  MAP_UARTCharPut(CONSOLE_UART, (r == 0 ? '+' : '-'));
  if (r != 0) return r;
  r = sl_FsRead(fh, 0, dst, fi.FileLen);
  if (r != fi.FileLen) return r;
  sl_FsClose(fh, NULL, NULL, 0);
  return 0;
}

void run(uint32_t base) {
  __asm(
      "ldr sp, [r0]\n"
      "add r0, r0, #4\n"
      "ldr r1, [r0]\n"
      "bx  r1");
  /* Not reached. */
}

static void crlflf() {
  MAP_UARTCharPut(CONSOLE_UART, '\r');
  MAP_UARTCharPut(CONSOLE_UART, '\n');
  MAP_UARTCharPut(CONSOLE_UART, '\n');
}

extern uint32_t _text_start; /* Our location. */

int main() {
  MAP_IntVTableBaseSet((unsigned long) &int_vectors[0]);
  MAP_IntMasterEnable();
  PRCMCC3200MCUInit();

  /* Console UART init. */
  MAP_PRCMPeripheralClkEnable(CONSOLE_UART_PERIPH, PRCM_RUN_MODE_CLK);
  MAP_PinTypeUART(PIN_55, PIN_MODE_3); /* PIN_55 -> UART0_TX */
  MAP_PinTypeUART(PIN_57, PIN_MODE_3); /* PIN_57 -> UART0_RX */
  MAP_UARTConfigSetExpClk(
      CONSOLE_UART, MAP_PRCMPeripheralClockGet(CONSOLE_UART_PERIPH),
      CONSOLE_BAUD_RATE,
      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  MAP_UARTFIFOLevelSet(CONSOLE_UART, UART_FIFO_TX1_8, UART_FIFO_RX4_8);
  MAP_UARTFIFODisable(CONSOLE_UART);

  crlflf();

  if (sl_Start(NULL, NULL, NULL) < 0) abort();
  MAP_UARTCharPut(CONSOLE_UART, 'S');

  int cidx = get_active_boot_cfg_idx();
  if (cidx < 0) abort();
  MAP_UARTCharPut(CONSOLE_UART, '0' + cidx);
  struct boot_cfg cfg;
  if (read_boot_cfg(cidx, &cfg) < 0) abort();

  uart_puts(cfg.app_image_file);
  MAP_UARTCharPut(CONSOLE_UART, '@');
  print_addr(cfg.app_load_addr);

  /*
   * Zero memory before loading.
   * This should provide proper initialisation for BSS, wherever it is.
   */
  uint32_t *pstart = (uint32_t *) 0x20000000;
  uint32_t *pend = (&_text_start - 0x100 /* our stack */);
  for (uint32_t *p = pstart; p < pend; p++) *p = 0;

  if (load_image(cfg.app_image_file, (_u8 *) cfg.app_load_addr) != 0) {
    abort();
  }

  MAP_UARTCharPut(CONSOLE_UART, '.');

  sl_Stop(0);
  print_addr(*(((uint32_t *) cfg.app_load_addr) + 1));
  crlflf();

  MAP_IntMasterDisable();
  MAP_IntVTableBaseSet(cfg.app_load_addr);

  run(cfg.app_load_addr); /* Does not return. */

  abort();

  return 0; /* not reached */
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *e) {
}

void SimpleLinkWlanEventHandler(SlWlanEvent_t *e) {
}

void SimpleLinkSockEventHandler(SlSockEvent_t *e) {
}

void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *e,
                                  SlHttpServerResponse_t *resp) {
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *e) {
}
