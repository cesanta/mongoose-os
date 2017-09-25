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

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "common/cs_dbg.h"
#include "common/str_util.h"

#include "mgos_debug.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_sys_config.h"
#include "mgos_uart.h"
#include "mgos_vfs_fs_spiffs.h"

#include "cc32xx_fs.h"
#include "cc32xx_main.h"
#include "cc32xx_vfs_fs_slfs.h"

#include "cc3220_vfs_dev_flash.h"

static bool cc3220_fs_init(const char *root_container_prefix) {
  return cc3220_vfs_dev_flash_register_type() &&
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
         cc32xx_fs_slfs_mount("/slfs");
}

int cc3220_init(bool pre) {
  if (pre) {
    Power_init();
    GPIO_init();
    SPI_init(); /* For NWP */
    UDMACC32XX_init();
    return 0;
  }
  if (!cc3220_fs_init("spiffs.img.0")) {
    LOG(LL_ERROR, ("FS init error"));
    return -1;
  }
  return 0;
}

int main(void) {
  cc32xx_main(cc3220_init); /* Does not return */

  return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
  (void) pcTaskName;
  (void) pxTask;

  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
  function is called if a stack overflow is detected. */
  taskDISABLE_INTERRUPTS();
  for (;;)
    ;
}
/*-----------------------------------------------------------*/

void PowerCC32XX_enterLPDS(void *arg) {
  (void) arg;
  assert(false);
}
