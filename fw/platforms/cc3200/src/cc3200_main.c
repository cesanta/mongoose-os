/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __TI_COMPILER_VERSION__
#include <unistd.h>
#endif

/* Driverlib includes */
#include "inc/hw_types.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin.h"
#include "driverlib/prcm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/utils.h"
#include "driverlib/wdt.h"

#include "common/platform.h"
#include "common/cs_dbg.h"

#include "simplelink.h"
#include "device.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_updater_common.h"
#include "mgos_vfs_fs_spiffs.h"

#include "cc32xx_fs.h"
#include "cc32xx_main.h"
#include "cc32xx_vfs_dev_slfs_container.h"
#include "cc32xx_vfs_fs_slfs.h"

#include "fw/platforms/cc3200/boot/lib/boot.h"

int g_boot_cfg_idx;
struct boot_cfg g_boot_cfg;
/* These are FreeRTOS hooks for various life situations. */
void vApplicationMallocFailedHook(void) {
  fprintf(stderr, "malloc failed\n");
  exit(123);
}

void vApplicationIdleHook(void) {
  /* Ho-hum. Twiddling our thumbs. */
}

void vApplicationStackOverflowHook(OsiTaskHandle *th, signed char *tn) {
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *e) {
}

/* Int vector table, defined in startup_gcc.c */
extern void (*const g_pfnVectors[])(void);

/* It may not be the best source of entropy, but it's better than nothing. */
static void cc3200_srand(void) {
  uint32_t r = 0, *p;
  for (p = (uint32_t *) 0x20000000; p < (uint32_t *) 0x20040000; p++) r ^= *p;
  srand(r);
}

static bool cc3200_fs_init(const char *root_container_prefix) {
  return cc32xx_vfs_dev_slfs_container_register_type() &&
         cc32xx_vfs_fs_slfs_register_type() &&
         mgos_vfs_fs_spiffs_register_type() &&
         cc32xx_fs_spiffs_container_mount("/", root_container_prefix) &&
         cc32xx_fs_slfs_mount("/slfs");
}

enum cc3200_init_result {
  CC3200_INIT_OK = 0,
  CC3200_INIT_FAILED_TO_START_NWP = -100,
  CC3200_INIT_FAILED_TO_READ_BOOT_CFG = -101,
  CC3200_INIT_FS_INIT_FAILED = -102,
  CC3200_INIT_MG_INIT_FAILED = -103,
  CC3200_INIT_UART_INIT_FAILED = -106,
  CC3200_INIT_UPDATE_FAILED = -107,
};

static int cc3200_init(bool pre) {
  if (pre) {
    cc3200_srand();
    return 0;
  }

  g_boot_cfg_idx = get_active_boot_cfg_idx();
  if (g_boot_cfg_idx < 0 || read_boot_cfg(g_boot_cfg_idx, &g_boot_cfg) < 0) {
    return CC3200_INIT_FAILED_TO_READ_BOOT_CFG;
  }

  LOG(LL_INFO,
      ("Boot cfg %d: 0x%llx, 0x%u, %s @ 0x%08x, %s", g_boot_cfg_idx,
       g_boot_cfg.seq, (unsigned int) g_boot_cfg.flags,
       g_boot_cfg.app_image_file, (unsigned int) g_boot_cfg.app_load_addr,
       g_boot_cfg.fs_container_prefix));

#if MGOS_ENABLE_UPDATER
  if (g_boot_cfg.flags & BOOT_F_FIRST_BOOT) {
    /* Tombstone the current config. If anything goes wrong between now and
     * commit, next boot will use the old one. */
    uint64_t saved_seq = g_boot_cfg.seq;
    g_boot_cfg.seq = BOOT_CFG_TOMBSTONE_SEQ;
    write_boot_cfg(&g_boot_cfg, g_boot_cfg_idx);
    g_boot_cfg.seq = saved_seq;
  }
#endif

  if (!cc3200_fs_init(g_boot_cfg.fs_container_prefix)) {
    LOG(LL_ERROR, ("FS init error"));
    return CC3200_INIT_FS_INIT_FAILED;
  } else {
    /*
     * We aim to maintain at most 3 FS containers at all times.
     * Delete inactive FS container in the inactive boot configuration.
     */
    struct boot_cfg cfg;
    int inactive_idx = (g_boot_cfg_idx == 0 ? 1 : 0);
    if (read_boot_cfg(inactive_idx, &cfg) >= 0) {
      cc32xx_vfs_dev_slfs_container_delete_inactive_container(
          cfg.fs_container_prefix);
    }
  }

#if MGOS_ENABLE_UPDATER
  if (g_boot_cfg.flags & BOOT_F_FIRST_BOOT) {
    LOG(LL_INFO, ("Applying update"));
    int r = mgos_upd_apply_update();
    if (r < 0) {
      LOG(LL_ERROR, ("Failed to apply update: %d", r));
      return CC3200_INIT_UPDATE_FAILED;
    }
  }
#endif

  return CC3200_INIT_OK;
}

void cc3200_nsleep100(uint32_t n);

int main(void) {
  MAP_IntVTableBaseSet((unsigned long) &g_pfnVectors[0]);
  mgos_nsleep100 = &cc3200_nsleep100;

  MAP_IntEnable(FAULT_SYSTICK);
  MAP_IntMasterEnable();

  cc32xx_main(cc3200_init); /* Does not return */

  return 0;
}
