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

#include <stdio.h>

#include "app.h"
#include "peripheral/peripheral.h"
#include "pic32_uart.h"
#include "pic32_ethernet.h"
#include "common/cs_dbg.h"
#include "mgos_init_internal.h"
#include "mgos_mongoose_internal.h"
#include "mgos_sys_config.h"

void APP_Initialize(void) {
  pic32_ethernet_init();
  pic32_uart_init();
  mongoose_init();

  enum mgos_init_result ir = mgos_init();
  if (ir != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("%s init error: %d", "MG", ir));
  }
}

/* Main application's state machine polled by SYS_Tasks */
void APP_Tasks(void) {
  pic32_ethernet_poll();
  mongoose_poll(0);
}
