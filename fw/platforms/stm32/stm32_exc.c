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

#include "mgos_core_dump.h"

#include "stm32_uart.h"

/*
 * There isn't much here, and here's why.
 * 1) Exceptions are routed to arm_exc_handler_top, which is written in asm,
 *    determines the stack pointer, populates most of the GDB frame and...
 * 2) ...hands off to arm_exc_handler_bottom, which prints exception info,
 *    dumps core and reboots.
 *
 * Platform code only needs to provide character printing and low-level reboot
 *functions.
 */

void mgos_cd_putc(int c) {
  stm32_uart_dputc(c);
}
