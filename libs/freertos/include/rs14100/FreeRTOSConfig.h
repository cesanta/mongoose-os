/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#pragma once  // no_extern_c_check

/* Ensure stdint is only used by the compiler, and not the assembler. */
#ifndef __ASSEMBLER__
#include <assert.h>
#include <stdint.h>
extern uint32_t SystemCoreClock;
//#define configASSERT(x) assert(x)
#include <core_cm4.h>
#endif

#define configCPU_CLOCK_HZ (SystemCoreClock)

#define configPRIO_BITS __NVIC_PRIO_BITS
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 2

#include "FreeRTOSConfigCommon.h"
