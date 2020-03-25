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

#include <string.h>

#include "common/platform.h"

#include "stm32_sdk_hal.h"
#include "stm32_system.h"

void (*stm32_int_vectors[256])(void)
    __attribute__((section(".bss.ram_int_vectors")));
extern const void *stm32_flash_int_vectors[2];

extern void arm_exc_handler_top(void);

void stm32_set_int_handler(int irqn, void (*handler)(void)) {
  stm32_int_vectors[irqn + 16] = handler;
}

void stm32_setup_int_vectors(void) {
  /* Move int vectors to RAM. */
  for (int i = 0; i < (int) ARRAY_SIZE(stm32_int_vectors); i++) {
    stm32_int_vectors[i] = arm_exc_handler_top;
  }
  memcpy(stm32_int_vectors, stm32_flash_int_vectors,
         sizeof(stm32_flash_int_vectors));
  SCB->VTOR = (uint32_t) &stm32_int_vectors[0];
}
