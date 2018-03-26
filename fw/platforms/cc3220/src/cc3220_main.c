/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <inc/hw_types.h>
#include <driverlib/prcm.h>
#include <driverlib/rom_map.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/dma/UDMACC32XX.h>
#include <ti/drivers/net/wifi/netutil.h>

#include "FreeRTOS.h"
#include "task.h"

#include "common/str_util.h"

#include "mgos_core_dump.h"
#include "mgos_hal.h"
#include "mgos_vfs.h"
#include "mgos_vfs_fs_spiffs.h"

#include "cc32xx_fs.h"
#include "cc32xx_main.h"
#include "cc32xx_vfs_fs_slfs.h"

#include "cc3220_vfs_dev_flash.h"

enum mgos_init_result mgos_fs_init(void) {
  if (!(cc3220_vfs_dev_flash_register_type() &&
         cc32xx_vfs_fs_slfs_register_type() &&
         mgos_vfs_fs_spiffs_register_type() &&
         mgos_vfs_mount(
             "/", MGOS_DEV_TYPE_FLASH,
             "{offset: " CS_STRINGIFY_MACRO(MGOS_FS_OFFSET) ", "
             "size: " CS_STRINGIFY_MACRO(MGOS_FS_SIZE) ", "
             "image: \"" MGOS_FS_IMG "\"}",
             MGOS_VFS_FS_TYPE_SPIFFS,
             "{bs: " CS_STRINGIFY_MACRO(MGOS_FS_BLOCK_SIZE) ", "
             "ps: " CS_STRINGIFY_MACRO(MGOS_FS_PAGE_SIZE) ", "
             "es: " CS_STRINGIFY_MACRO(MGOS_FS_ERASE_SIZE) "}") &&
         cc32xx_fs_slfs_mount("/slfs"))) {
    return MGOS_INIT_FS_INIT_FAILED;
  }
  return MGOS_INIT_OK;
}

enum mgos_init_result cc32xx_pre_nwp_init(void) {
  Power_init();
  GPIO_init();
  SPI_init(); /* For NWP */
  UDMACC32XX_init();
  return MGOS_INIT_OK;
}

enum mgos_init_result cc32xx_init(void) {
  _u16 len16 = 4;
  uint32_t seed = 0;
  sl_NetUtilGet(SL_NETUTIL_TRUE_RANDOM, 0, (uint8_t *) &seed, &len16);
  return MGOS_INIT_OK;
}

int main(void) {
  cc32xx_main(); /* Does not return */
  return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
  (void) pxTask;
  mgos_cd_printf("%s: stack overflow\n", pcTaskName);
  abort();
}

void PowerCC32XX_enterLPDS(void *arg) {
  (void) arg;
  abort();
}
