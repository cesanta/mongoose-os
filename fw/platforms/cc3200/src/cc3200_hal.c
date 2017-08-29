/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"

#include "driverlib/prcm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

#include "common/platform.h"
#include "common/cs_dbg.h"

#include "simplelink.h"
#include "device.h"
#include "oslib/osi.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "mgos_hal.h"
#include "mgos_vfs.h"

#include "fw/platforms/cc3200/src/config.h"
#include "fw/platforms/cc3200/src/cc3200_fs.h"

size_t mgos_get_fs_memory_usage(void) {
  return 0; /* Not even sure if it's possible to tell. */
}

void mgos_system_restart(int exit_code) {
  (void) exit_code;
  if (exit_code != 100) {
    mgos_vfs_umount_all();
    sl_Stop(50 /* ms */);
  }
  /* Turns out to be not that easy. In particular, using *Reset functions is
   * not a good idea.
   * https://e2e.ti.com/support/wireless_connectivity/f/968/p/424736/1516404
   * Instead, the recommended way is to enter hibernation with immediate wakeup.
   */
  MAP_PRCMHibernateIntervalSet(328 /* 32KHz ticks, 100 ms */);
  MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);
  MAP_PRCMHibernateEnter();
}

uint32_t mgos_bitbang_n100_cal;
void (*mgos_nsleep100)(uint32_t n);
void cc3200_nsleep100(uint32_t n) {
  /* TODO(rojer) */
}

#ifndef MGOS_HAVE_WIFI
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *e) {
  (void) e;
}
void SimpleLinkWlanEventHandler(SlWlanEvent_t *e) {
  (void) e;
}
#endif
