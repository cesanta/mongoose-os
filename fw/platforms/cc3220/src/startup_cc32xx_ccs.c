/*
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>

#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_common_reg.h>

#include <ti/devices/cc32xx/driverlib/interrupt.h>
#include <ti/devices/cc32xx/inc/hw_apps_rcm.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>

#include "cc32xx_exc.h"

void resetISR(void);

//*****************************************************************************
//
// External declaration for the reset handler that is to be called when the
// processor is started
//
//*****************************************************************************
extern void _c_int00(void);
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

//*****************************************************************************
//
// Linker variable that marks the top of the stack.
//
//*****************************************************************************
extern unsigned long __STACK_END;

//*****************************************************************************
// The vector table.  Note that the proper constructs must be placed on this to
// ensure that it ends up at physical address 0x0000.0000 or at the start of
// the program if located at a start address other than 0.
//
//*****************************************************************************
#pragma RETAIN(resetVectors)
#pragma DATA_SECTION(resetVectors, ".resetVecs")
void (*const resetVectors[16])(void) = {
    (void (*)(void))((unsigned long) &__STACK_END),
    // The initial stack pointer
    resetISR,                       // The reset handler
    cc32xx_nmi_handler,             /* The NMI handler */
    cc32xx_hard_fault_handler_top,  /* The hard fault handler */
    cc32xx_mem_fault_handler_top,   /* The MPU fault handler */
    cc32xx_bus_fault_handler_top,   /* The hard fault handler */
    cc32xx_usage_fault_handler_top, /* The usage fault handler */
    0,                              // Reserved
    0,                              // Reserved
    0,                              // Reserved
    0,                              // Reserved
    vPortSVCHandler,                // SVCall handler
    cc32xx_unhandled_int,           /* Debug monitor handler */
    0,                              // Reserved
    xPortPendSVHandler,             // The PendSV handler
    xPortSysTickHandler             // The SysTick handler
};

#pragma DATA_SECTION(ramVectors, ".ramVecs")
static unsigned long ramVectors[256];

//*****************************************************************************
//
// Copy the first 16 vectors from the read-only/reset table to the runtime
// RAM table. Fill the remaining vectors with a stub. This vector table will
// be updated at runtime.
//
//*****************************************************************************
void initVectors(void) {
  int i;

  /* Copy from reset vector table into RAM vector table */
  memcpy(ramVectors, resetVectors, 16 * 4);

  /* fill remaining vectors with default handler */
  for (i = 16; i < 256; i++) {
    ramVectors[i] = (unsigned long) cc32xx_unhandled_int;
  }

  /* Set vector table base */
  MAP_IntVTableBaseSet((unsigned long) &ramVectors[0]);

  /* Enable Processor */
  MAP_IntMasterEnable();
  MAP_IntEnable(FAULT_SYSTICK);
}

//*****************************************************************************
//
// This is the code that gets called when the processor first starts execution
// following a reset event.  Only the absolutely necessary set is performed,
// after which the application supplied entry() routine is called.  Any fancy
// actions (such as making decisions based on the reset cause register, and
// resetting the bits in that register) are left solely in the hands of the
// application.
//
//*****************************************************************************
void resetISR(void) {
  /*
   * Set stack pointer based on the stack value stored in the vector table.
   * This is necessary to ensure that the application is using the correct
   * stack when using a debugger since a reset within the debugger will
   * load the stack pointer from the bootloader's vector table at address '0'.
   */
  __asm(
      " .global resetVectorAddr\n"
      " ldr r0, resetVectorAddr\n"
      " ldr r0, [r0]\n"
      " mov sp, r0\n"
      " bl initVectors");

  /* Jump to the CCS C Initialization Routine. */
  __asm(
      " .global _c_int00\n"
      " b.w     _c_int00");

  _Pragma("diag_suppress 1119");
  __asm("resetVectorAddr: .word resetVectors");
  _Pragma("diag_default 1119");
}
