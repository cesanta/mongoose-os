/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_system.h"

#include <stdlib.h>
#include <stdbool.h>

#include "mgos_hal.h"
#include "mgos_hooks.h"
#include "mgos_vfs.h"
#ifdef MGOS_HAVE_WIFI
#include "mgos_wifi.h"
#endif

void mgos_system_restart(void) {
  mgos_hook_trigger(MGOS_HOOK_SYSTEM_RESTART, NULL);
  mgos_vfs_umount_all();
#ifdef MGOS_HAVE_WIFI
  mgos_wifi_disconnect();
#endif
  LOG(LL_INFO, ("Restarting"));
  mgos_debug_flush();
  mgos_dev_system_restart();
}
