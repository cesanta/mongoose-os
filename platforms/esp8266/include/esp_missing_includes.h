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

#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
// Can't be enabled yet due to SDK issues. TODO(rojer): Enable.
// extern "C" {
#endif

void pp_soft_wdt_init(void);
void pp_soft_wdt_stop(void);
void pp_soft_wdt_feed(void);
void pp_soft_wdt_restart(void);

void Cache_Read_Disable(void);
void Cache_Read_Enable(uint32_t, uint32_t, uint32_t);
void Cache_Read_Disable_2(void);
void Cache_Read_Enable_2(void);
void Cache_Read_Enable_New(void);

int SPIEraseBlock(uint32_t block);
uint32_t SPIRead(uint32_t addr, void *dst, uint32_t size);

uint8_t rom_i2c_readReg(uint32_t block, uint32_t host_id, uint32_t reg_add);
void rom_i2c_writeReg(uint32_t block, uint32_t host_id, uint32_t reg_add,
                      uint8_t data);

#ifndef RTOS_SDK

#include <ets_sys.h>

/* There are no declarations for these anywhere in the SDK (as of 1.2.0). */
void system_restart_local(void);
int os_printf_plus(const char *format, ...);

void ets_wdt_init(void);
void ets_wdt_enable(uint32_t mode, uint32_t arg1, uint32_t arg2);
void ets_wdt_disable(void);
void ets_wdt_restore(uint32_t mode);
uint32_t ets_wdt_get_mode(void);

void _xtos_l1int_handler(void);
void _xtos_set_exception_handler();
void xthal_set_intenable(unsigned);

#else /* !RTOS_SDK */

#define BIT(nr) (1UL << (nr))
void system_soft_wdt_feed(void);
void system_soft_wdt_restart(void);
void ets_putc(char c);

#endif /* RTOS_SDK */

void _ResetVector(void);

#ifdef __cplusplus
//}
#endif
