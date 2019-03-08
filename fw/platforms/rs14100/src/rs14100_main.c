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

#include "system_RS1xxxx.h"
#include "rsi_chip.h"

typedef struct {
  uint8_t port;
  uint8_t pin;
} PORT_PIN_T;

static const PORT_PIN_T ledBits[] = {{0, 0}, {0, 2}, {0, 12}};
static const uint32_t ledBitsCnt = sizeof(ledBits) / sizeof(PORT_PIN_T);

void RSI_Board_Init(void) {
  unsigned int i;

  for (i = 0; i < ledBitsCnt; i++) {
    /*Set the GPIO pin MUX */
    RSI_EGPIO_SetPinMux(EGPIO1, ledBits[i].port, ledBits[i].pin, 0);
    /*Set GPIO direction*/
    RSI_EGPIO_SetDir(EGPIO1, ledBits[i].port, ledBits[i].pin, 0);
    /*Receive enable */
    RSI_EGPIO_PadReceiverEnable(((ledBits[i].port) * 16) + ledBits[i].pin);
  }
  /*Enable PAD selection*/
  RSI_EGPIO_PadSelectionEnable(1);
}

void RSI_Board_LED_Set(int x, int y) {
  RSI_EGPIO_SetPin(EGPIO1, ledBits[x].port, ledBits[x].pin, y);
}

void RSI_Board_LED_Toggle(int x) {
  RSI_EGPIO_TogglePort(EGPIO1, ledBits[x].port, (1 << ledBits[x].pin));
}

void SysTick_Handler1(void) {
  RSI_Board_LED_Toggle(1);
}

int main(void) {
  SystemInit();
  SystemCoreClockUpdate();
  RSI_Board_Init();
  RSI_Board_LED_Set(0, 1);
  SysTick_Config(SystemCoreClock / 10);
  while (1) {
    // uint32_t tc = SysTick->VAL;
    // while (SysTick->VAL == tc);
    // RSI_Board_LED_Toggle(0);
  }
}
