/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Power.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "common/cs_dbg.h"

#include "mgos_debug.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_sys_config.h"
#include "mgos_uart.h"

#include "cc32xx_main.h"

#ifndef MGOS_DEBUG_UART
#define MGOS_DEBUG_UART 0
#endif
#ifndef MGOS_DEBUG_UART_BAUD_RATE
#define MGOS_DEBUG_UART_BAUD_RATE 115200
#endif

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

static int pulse = 0;

void vApplicationTickHook(void) {
  static int x = 0;
  if (!pulse) return;
  GPIO_write( 2, x );
  x ^= 1;
}

int cc3220_init(void) {
//  mongoose_init();
  if (mgos_debug_uart_init() != MGOS_INIT_OK) {
    return -1;
  }
  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }
  LOG(LL_INFO, ("Mongoose OS %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("RAM: %d total, %d free", mgos_get_heap_size(),
                mgos_get_free_heap_size()));
  return 0;
}

// XXX: Temporary.
struct sys_config *get_cfg(void) {
  return NULL;
}

int main(void) {
  PRCMCC3200MCUInit();
  Power_init();
  GPIO_init();

  if (!MAP_PRCMRTCInUseGet()) {
    MAP_PRCMRTCInUseSet();
    MAP_PRCMRTCSet(0, 0);
  }

  cc32xx_main(cc3220_init);  /* Does not return */

  return 0;
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
  ( void ) pcTaskName;
  ( void ) pxTask;

  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
  function is called if a stack overflow is detected. */
  taskDISABLE_INTERRUPTS();
  for( ;; );
}
/*-----------------------------------------------------------*/

/* Catch asserts so the file and line number of the assert can be viewed. */
void vMainAssertCalled( const char *pcFileName, uint32_t ulLineNumber )
{
    taskENTER_CRITICAL();
    for( ;; )
    {
        /* Use the variables to prevent compiler warnings and in an attempt to
        ensure they can be viewed in the debugger.  If the variables get
        optimised away then set copy their values to file scope or globals then
        view the variables they are copied to. */
        ( void ) pcFileName;
        ( void ) ulLineNumber;
    }
}

void PowerCC32XX_enterLPDS(void *arg) {
  (void) arg;
  assert(false);
}
