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

#include "mgos_gpio.h"

#include "system_RS1xxxx.h"
#include "rsi_chip.h"

#define LED_ON 0
#define LED1_PIN RS14100_ULP_GPIO(0) /* TRI LED green, 0 = on, 1 = off */
#define LED2_PIN RS14100_ULP_GPIO(2) /* TRI LED blue,  0 = on, 1 = off */

#define TEST_PIN RS14100_HP_GPIO(66)

// 31: debug

void SysTick_Handler1(void) {
  mgos_gpio_toggle(RS14100_UULP_GPIO(2));
  mgos_gpio_write(RS14100_ULP_GPIO(5), mgos_gpio_read(RS14100_HP_GPIO(50)));
  mgos_gpio_toggle(TEST_PIN);
}

extern void arm_exc_handler_top(void);
extern const void *flash_int_vectors[60];
static void (*int_vectors[256])(void)
    __attribute__((section(".ram_int_vectors")));

void rs14100_set_int_handler(int irqn, void (*handler)(void)) {
  int_vectors[irqn + 16] = handler;
}

int main(void) {
  /* Move int vectors to RAM. */
  for (int i = 0; i < (int) ARRAY_SIZE(int_vectors); i++) {
    int_vectors[i] = 0;  // arm_exc_handler_top;
  }
  memcpy(int_vectors, flash_int_vectors, sizeof(flash_int_vectors));
  // rs14100_set_int_handler(SVCall_IRQn, SVC_Handler);
  // rs14100_set_int_handler(PendSV_IRQn, PendSV_Handler);
  rs14100_set_int_handler(SysTick_IRQn, SysTick_Handler1);
  SCB->VTOR = (uint32_t) &int_vectors[0];

  SystemInit();
  SystemCoreClockUpdate();
  mgos_gpio_setup_output(LED1_PIN, !LED_ON);
  mgos_gpio_setup_output(LED2_PIN, !LED_ON);
  mgos_gpio_setup_output(TEST_PIN, 1);
  mgos_gpio_setup_input(RS14100_HP_GPIO(50), MGOS_GPIO_PULL_UP);
  mgos_gpio_setup_output(RS14100_ULP_GPIO(5), 1);
  mgos_gpio_setup_output(RS14100_UULP_GPIO(2), 1);
  SysTick_Config(SystemCoreClock / 10);
  while (1) {
    // uint32_t tc = SysTick->VAL;
    // while (SysTick->VAL == tc);
    // RSI_Board_LED_Toggle(0);
  }
}
