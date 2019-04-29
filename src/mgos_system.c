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

#include "mgos_system.h"

#include <stdbool.h>
#include <stdlib.h>

#include "common/cs_dbg.h"

#include "mgos_debug.h"
#include "mgos_event.h"
#include "mgos_hal.h"
#include "mgos_hw_timers_hal.h"
#include "mgos_vfs.h"
#ifdef MGOS_HAVE_WIFI
#include "mgos_wifi.h"
#endif

void mgos_system_restart(void) {
  mgos_event_trigger(MGOS_EVENT_REBOOT, NULL);
  mgos_vfs_umount_all();
  mgos_hw_timers_deinit();
#ifdef MGOS_HAVE_WIFI
  mgos_wifi_disconnect();
  mgos_wifi_deinit();
#endif
  LOG(LL_INFO, ("Restarting"));
  mgos_debug_flush();
  mgos_dev_system_restart();
}
