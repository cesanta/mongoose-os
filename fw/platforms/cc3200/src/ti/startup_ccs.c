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

#include <stdint.h>

#include "cc32xx_exc.h"
#include "mgos_hal.h"

extern void arm_exc_handler_top(void);
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

/* Linker variable that marks the top of the stack. */
extern unsigned long __STACK_END;

extern void _c_int00(void);

#pragma DATA_SECTION(g_pfnVectors, ".intvecs")
void (*const g_pfnVectors[256])(void) = {
    (void (*)(void))(&__STACK_END), /* The initial stack pointer */
    _c_int00,                       /* The reset handler */
    arm_exc_handler_top,            /* The NMI handler */
    arm_exc_handler_top,            /* The hard fault handler */
    arm_exc_handler_top,            /* The MPU fault handler */
    arm_exc_handler_top,            /* The hard fault handler */
    arm_exc_handler_top,            /* The usage fault handler */
    0,                              /* Reserved */
    0,                              /* Reserved */
    0,                              /* Reserved */
    0,                              /* Reserved */
    vPortSVCHandler,                /* SVCall handler */
    arm_exc_handler_top,            /* Debug monitor handler */
    0,                              /* Reserved */
    xPortPendSVHandler,             /* The PendSV handler */
    xPortSysTickHandler,            /* The SysTick handler */
    arm_exc_handler_top,            /* GPIO Port A0 */
    arm_exc_handler_top,            /* GPIO Port A1 */
    arm_exc_handler_top,            /* GPIO Port A2 */
    arm_exc_handler_top,            /* GPIO Port A3 */
    0,                              /* Reserved */
    arm_exc_handler_top,            /* UART0 Rx and Tx */
    arm_exc_handler_top,            /* UART1 Rx and Tx */
    0,                              /* Reserved */
    arm_exc_handler_top,            /* I2C0 Master and Slave */
    0, 0, 0, 0, 0,                  /* Reserved */
    arm_exc_handler_top,            /* ADC Channel 0 */
    arm_exc_handler_top,            /* ADC Channel 1 */
    arm_exc_handler_top,            /* ADC Channel 2 */
    arm_exc_handler_top,            /* ADC Channel 3 */
    arm_exc_handler_top,            /* Watchdog Timer */
    arm_exc_handler_top,            /* Timer 0 subtimer A */
    arm_exc_handler_top,            /* Timer 0 subtimer B */
    arm_exc_handler_top,            /* Timer 1 subtimer A */
    arm_exc_handler_top,            /* Timer 1 subtimer B */
    arm_exc_handler_top,            /* Timer 2 subtimer A */
    arm_exc_handler_top,            /* Timer 2 subtimer B  */
    0, 0, 0, 0,                     /* Reserved */
    arm_exc_handler_top,            /* Flash */
    0, 0, 0, 0, 0,                  /* Reserved */
    arm_exc_handler_top,            /* Timer 3 subtimer A */
    arm_exc_handler_top,            /* Timer 3 subtimer B */
    0, 0, 0, 0, 0, 0, 0, 0, 0,      /* Reserved */
    arm_exc_handler_top,            /* uDMA Software Transfer */
    arm_exc_handler_top,            /* uDMA Error */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    arm_exc_handler_top,            /* SHA */
    0, 0,                           /* Reserved */
    arm_exc_handler_top,            /* AES */
    0,                              /* Reserved */
    arm_exc_handler_top,            /* DES */
    0, 0, 0, 0, 0,                  /* Reserved */
    arm_exc_handler_top,            /* SDHost */
    0,                              /* Reserved */
    arm_exc_handler_top,            /* I2S */
    0,                              /* Reserved */
    arm_exc_handler_top,            /* Camera */
    0, 0, 0, 0, 0, 0, 0,            /* Reserved */
    arm_exc_handler_top,            /* NWP to APPS Interrupt */
    arm_exc_handler_top,            /* Power, Reset and Clock module */
    0, 0,                           /* Reserved */
    arm_exc_handler_top,            /* Shared SPI */
    arm_exc_handler_top,            /* Generic SPI */
    arm_exc_handler_top,            /* Link SPI */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* Reserved */
    0, 0                            /* Reserved */
};
