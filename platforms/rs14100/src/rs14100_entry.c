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

#include <stdint.h>
#include <string.h>

extern int main(void);
extern void arm_exc_handler_top(void);

extern uint32_t _stack;
extern uint32_t _bss_start, _bss_end;
extern uint32_t _data_start, _data_end, _data_flash_start;

/* The main entry point of the program */
void rs14100_entry(void) {
  memset(&_bss_start, 0, ((char *) &_bss_end - (char *) &_bss_start));
  memcpy(&_data_start, &_data_flash_start,
         ((char *) &_data_end - (char *) &_data_start));
  main();
  while (1) {
  }
}

extern uint32_t _stack;
extern void arm_exc_handler_top(void);

/* Initial int vectors, in flash */
const __attribute__((
    section(".flash_int_vectors"))) void *flash_int_vectors[60] = {
    &_stack,  // ROM boot loader stack pointer.
    rs14100_entry,
    0,           // NMI_Handler
    0,           // HardFault_Handler
    0,           // MemManage_Handler
    0,           // BusFault_Handler
    0,           // UsageFault_Handler
    0, 0, 0, 0,  // Reserved
    0,           // SVC_Handler
    0,           // DebugMon_Handler
    0,           // Reserved
    0,           // PendSV_Handler
    0,           // SysTick_Handler
    // External interrupts
    0,                                                        // VAD interrupt
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // ULP (19)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // Sleep sensor (10)
    &_stack,
    0,  // RPDMA
    rs14100_entry,
    0,  // UDMA
    0,  // SCT
    0,  // HIF1
    0,  // HIF2
    0,  // SIO
    0,  // UART1
    0,  // UART2
    0,  // RSI_PS_Restore - something to do with waking up from sleep and
        // re-enabling flash
    0,  // GPIO Wakeup
    0,  // I2C
    (void *) 0x10ad10ad,  // Magic value for BL to consider app valid.
    /* There are more vectors, but we don't need them here, before moving to
       RAM. */
};
