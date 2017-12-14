/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_system.h"

#include <stdlib.h>
#include <stdbool.h>

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
