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

#include "common/platform.h"

#include "mgos_hal_freertos_internal.h"
#include "mgos_core_dump.h"
#include "mgos_gpio.h"
#include "mgos_system.h"
#include "mgos_uart.h"

#include "rs14100_sdk.h"

#define LED_ON 0
#define LED1_PIN RS14100_ULP_GPIO(0) /* TRI LED green, 0 = on, 1 = off */
#define LED2_PIN RS14100_ULP_GPIO(2) /* TRI LED blue,  0 = on, 1 = off */

// 31: debug

extern void arm_exc_handler_top(void);
extern const void *flash_int_vectors[60];
static void (*int_vectors[256])(void)
    __attribute__((section(".ram_int_vectors")));

void rs14100_set_int_handler(int irqn, void (*handler)(void)) {
  int_vectors[irqn + 16] = handler;
}

#define ICACHE_BASE 0x20280000
#define ICACHE_CTRL_REG (*((volatile uint32_t *) (ICACHE_BASE + 0x14)))
#define ICACHE_XLATE_REG (*((volatile uint32_t *) (ICACHE_BASE + 0x24)))

uint32_t SystemCoreClockMHZ = 0;

static void rs14100_enable_icache(void) {
  // Enable instruction cache.
  M4CLK->CLK_ENABLE_SET_REG1_b.ICACHE_CLK_ENABLE_b = 1;
  M4CLK->CLK_ENABLE_SET_REG1_b.ICACHE_CLK_2X_ENABLE_b = 1;
  // Per HRM, only required for 120MHz+, but we do it always, just in case.
  ICACHE_XLATE_REG |= (1 << 21);
  ICACHE_CTRL_REG = 0x1;  // 4-Way assoc, enable.
}

enum mgos_init_result mgos_hal_freertos_pre_init(void) {
  return MGOS_INIT_OK;
}

int main(void) {
  /* Move int vectors to RAM. */
  for (int i = 0; i < (int) ARRAY_SIZE(int_vectors); i++) {
    int_vectors[i] = arm_exc_handler_top;
  }
  memcpy(int_vectors, flash_int_vectors, sizeof(flash_int_vectors));
  SCB->VTOR = (uint32_t) &int_vectors[0];

  SystemInit();
  SystemCoreClockUpdate();
  SystemCoreClockMHZ = SystemCoreClock / 1000000;
  rs14100_enable_icache();

  // mgos_gpio_setup_output(LED1_PIN, !LED_ON);
  // mgos_gpio_setup_output(LED2_PIN, !LED_ON);

  NVIC_SetPriorityGrouping(3 /* NVIC_PRIORITYGROUP_4 */);
  NVIC_SetPriority(MemoryManagement_IRQn, 0);
  NVIC_SetPriority(BusFault_IRQn, 0);
  NVIC_SetPriority(UsageFault_IRQn, 0);
  NVIC_SetPriority(DebugMonitor_IRQn, 0);
  rs14100_set_int_handler(SVCall_IRQn, SVC_Handler);
  rs14100_set_int_handler(PendSV_IRQn, PendSV_Handler);
  rs14100_set_int_handler(SysTick_IRQn, SysTick_Handler);
  mgos_hal_freertos_run_mgos_task(true /* start_scheduler */);
  /* not reached */
  abort();
}
