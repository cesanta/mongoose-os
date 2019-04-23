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

#include <inc/hw_types.h>
#include <driverlib/prcm.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <ti/drivers/net/wifi/netutil.h>

#include "common/cs_dbg.h"
#include "common/platform.h"

#include "mgos_hal.h"
#include "cc32xx_exc.h"

void mgos_dev_system_restart(void) {
  sl_DeviceDisable(); /* Turn off NWP */
  MAP_PRCMMCUReset(true /* bIncludeSubsystem */);
  // Not reached.
  while (1) {
  }
}

void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *e) {
  cc32xx_exc_printf(
      "SL fatal error 0x%x assert 0x%x,0x%x no_ack 0x%x timeout 0x%x", e->Id,
      e->Data.DeviceAssert.Code, e->Data.DeviceAssert.Value,
      e->Data.NoCmdAck.Code, e->Data.CmdTimeout.Code);
  mgos_system_restart();
}

int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  _u16 len16 = len;
  return sl_NetUtilGet(SL_NETUTIL_TRUE_RANDOM, 0, buf, &len16);
}

#ifndef MGOS_HAVE_WIFI
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *e) {
  (void) e;
}
void SimpleLinkWlanEventHandler(SlWlanEvent_t *e) {
  (void) e;
}
#endif
