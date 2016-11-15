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

#include "fw/platforms/cc3200/src/config.h"

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

#if MIOT_DEBUG_UART == 0
#define DEBUG_UART_BASE UARTA0_BASE
#define DEBUG_UART_PERIPH PRCM_UARTA0
#elif MIOT_DEBUG_UART == 1
#define DEBUG_UART_BASE UARTA1_BASE
#define DEBUG_UART_PERIPH PRCM_UARTA1
#else
#define NO_DEBUG
#endif

/* Int vector table, defined in startup_gcc.c */
extern void (*const int_vectors[])(void);

void dbg_putc(char c) {
#ifndef NO_DEBUG
  MAP_UARTCharPut(DEBUG_UART_BASE, c);
#else
  (void) c;
#endif
}

void dbg_puts(const char *s) {
  for (; *s != '\0'; s++) dbg_putc(*s);
}

void abort(void) {
  dbg_putc('X');
  while (1) {
  }
}

static void print_addr(uint32_t addr) {
  char buf[10];
  __utoa(addr, buf, 16);
  dbg_puts(buf);
}

int load_image(const char *fn, _u8 *dst) {
  _i32 fh;
  SlFsFileInfo_t fi;
  _i32 r = sl_FsGetInfo((const _u8 *) fn, 0, &fi);
  dbg_putc(r == 0 ? '+' : '-');
  if (r != 0) return r;
  {
    char buf[20];
    __utoa(fi.FileLen, buf, 10);
    dbg_puts(buf);
  }
  r = sl_FsOpen((const _u8 *) fn, FS_MODE_OPEN_READ, NULL, &fh);
  dbg_putc(r == 0 ? '+' : '-');
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

extern uint32_t _text_start; /* Our location. */

int main(void) {
  MAP_IntVTableBaseSet((unsigned long) &int_vectors[0]);
  MAP_IntMasterEnable();
  PRCMCC3200MCUInit();

/* Console UART init. */
#ifndef NO_DEBUG
  MAP_PRCMPeripheralClkEnable(DEBUG_UART_PERIPH, PRCM_RUN_MODE_CLK);
#if MIOT_DEBUG_UART == 0
  MAP_PinTypeUART(PIN_55, PIN_MODE_3); /* UART0_TX */
  MAP_PinTypeUART(PIN_57, PIN_MODE_3); /* UART0_RX */
#else
  MAP_PinTypeUART(PIN_07, PIN_MODE_5); /* UART1_TX */
  MAP_PinTypeUART(PIN_08, PIN_MODE_5); /* UART1_RX */
#endif
  MAP_UARTConfigSetExpClk(
      DEBUG_UART_BASE, MAP_PRCMPeripheralClockGet(DEBUG_UART_PERIPH),
      MIOT_DEBUG_UART_BAUD_RATE,
      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  MAP_UARTFIFOLevelSet(DEBUG_UART_BASE, UART_FIFO_TX1_8, UART_FIFO_RX4_8);
  MAP_UARTFIFODisable(DEBUG_UART_BASE);
#endif

  dbg_puts("\r\n\n");

  if (sl_Start(NULL, NULL, NULL) < 0) abort();
  dbg_putc('S');

  int cidx = get_active_boot_cfg_idx();
  if (cidx < 0) abort();
  dbg_putc('0' + cidx);
  struct boot_cfg cfg;
  if (read_boot_cfg(cidx, &cfg) < 0) abort();

  dbg_puts(cfg.app_image_file);
  dbg_putc('@');
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

  dbg_putc('.');

  sl_Stop(0);
  print_addr(*(((uint32_t *) cfg.app_load_addr) + 1));
  dbg_puts("\r\n\n");

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
