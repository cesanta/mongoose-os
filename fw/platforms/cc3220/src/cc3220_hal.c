/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <inc/hw_types.h>
#include <driverlib/prcm.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>

#include "mgos_hal.h"

void mgos_system_restart(int exit_code) {
  MAP_PRCMMCUReset(true /* bIncludeSubsystem */);
}

void mgos_wdt_feed(void) {
  // TODO(rojer)
}

void mgos_wdt_set_timeout(int secs) {
  // TODO(rojer)
}

void mgos_wdt_enable(void) {
  // TODO(rojer)
}

void mgos_wdt_disable(void) {
  // TODO(rojer)
}
