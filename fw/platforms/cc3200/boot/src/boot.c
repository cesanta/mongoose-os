/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <inttypes.h>

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

#define SYS_CLK 80000000
#define CONSOLE_BAUD_RATE 115200
#define CONSOLE_UART UARTA0_BASE
#define CONSOLE_UART_INT INT_UARTA0
#define CONSOLE_UART_PERIPH PRCM_UARTA0

/* Int vector table, defined in startup_gcc.c */
extern void (*const int_vectors[])(void);

void read_file(const char *fn) {
  _i32 fh;
  SlFsFileInfo_t fi;
  _i32 r = sl_FsOpen((const _u8 *) fn, FS_MODE_OPEN_READ, NULL, &fh);
  _u8 buf[128];
  if (r != 0) return;
  r = sl_FsGetInfo((const _u8 *) fn, 0, &fi);
  if (r != 0) return;
  r = sl_FsRead(fh, 0, buf, sizeof(buf));
  if (r != 0) return;
}

int main() {
  MAP_IntVTableBaseSet((unsigned long) &int_vectors[0]);
  MAP_IntEnable(FAULT_SYSTICK);
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

  MAP_UARTCharPut(CONSOLE_UART, '?');
  sl_Start(NULL, NULL, NULL);
  MAP_UARTCharPut(CONSOLE_UART, '!');

  read_file("test.json");

  return 0;
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
