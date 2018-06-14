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

#include <inc/hw_types.h>
#include <driverlib/prcm.h>
#include <driverlib/interrupt.h>
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

#include "cc32xx_fs.h"
#include "cc32xx_main.h"

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

extern void arm_exc_handler_top(void);
extern void _c_int00(void);
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
// Linker variable that marks the top of the stack.
extern unsigned long __STACK_END;

#pragma DATA_SECTION(int_vecs, ".int_vecs")
void (*int_vecs[256])(void) = {
    (void (*)(void))((unsigned long) &__STACK_END),
    _c_int00,             /* The reset handler */
    arm_exc_handler_top,  /* The NMI handler */
    arm_exc_handler_top,  /* The hard fault handler */
    arm_exc_handler_top,  /* The MPU fault handler */
    arm_exc_handler_top,  /* The hard fault handler */
    arm_exc_handler_top,  /* The usage fault handler */
    arm_exc_handler_top,  // Reserved
    arm_exc_handler_top,  // Reserved
    arm_exc_handler_top,  // Reserved
    arm_exc_handler_top,  // Reserved
    vPortSVCHandler,      // SVCall handler
    arm_exc_handler_top,  /* Debug monitor handler */
    arm_exc_handler_top,  // Reserved
    xPortPendSVHandler,   // The PendSV handler
    xPortSysTickHandler,  // The SysTick handler
    /* 0x10 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0x20 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0x30 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0x40 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0x50 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0x60 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0x70 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0x80 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0x90 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0xa0 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0xb0 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0xc0 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0xd0 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0xe0 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
    /* 0xf0 */
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top, arm_exc_handler_top, arm_exc_handler_top,
    arm_exc_handler_top,
};

int main(void) {
  /* auto_init code in _c_int00 has copied the vectors table to RAM,
   * all that remains is for us to switch to it. */
  MAP_IntVTableBaseSet((unsigned long) &int_vecs[0]);
  MAP_IntMasterEnable();
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
