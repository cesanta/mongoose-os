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

#include "mgos_debug.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_sys_config.h"
#include "mgos_uart.h"

#include "cc32xx_exc.h"
#include "cc32xx_main.h"

static int pulse = 0;

void vApplicationTickHook(void) {
  static int x = 0;
  if (!pulse) return;
  GPIO_write(2, x);
  x ^= 1;
}

int cc3220_init(bool pre) {
  if (pre) {
    Power_init();
    GPIO_init();
    SPI_init(); /* For NWP */
    UDMACC32XX_init();
  }
  return 0;
}

// XXX: Temporary.
struct sys_config *get_cfg(void) {
  return NULL;
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
