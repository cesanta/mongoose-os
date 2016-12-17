/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include "app.h"
#include "peripheral/peripheral.h"
#include "pic32_uart.h"
#include "pic32_ethernet.h"
#include "common/cs_dbg.h"
#include "fw/src/miot_init.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_sys_config.h"

void APP_Initialize(void) {
  pic32_ethernet_init();
  pic32_uart_init();
  mongoose_init();

  enum miot_init_result ir = miot_init();
  if (ir != MIOT_INIT_OK) {
    LOG(LL_ERROR, ("%s init error: %d", "MG", ir));
  }
}

/* Main application's state machine polled by SYS_Tasks */
void APP_Tasks(void) {
  pic32_ethernet_poll();
  mongoose_poll(0);
}
