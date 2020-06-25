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
#include <string.h>

#include "mgos_gpio.h"

extern int main(void);
extern void arm_exc_handler_top(void);

extern uint32_t _stack;
extern uint32_t _bss_start, _bss_end;
extern uint32_t _data_start, _data_end, _data_flash_start;
extern uint32_t _heap_start, _heap_end;

/* The main entry point of the program */
void stm32_entry(void) {
  memset(&_bss_start, 0, ((char *) &_bss_end - (char *) &_bss_start));
  memcpy(&_data_start, &_data_flash_start,
         ((char *) &_data_end - (char *) &_data_start));
  memset(&_heap_start, 0, ((char *) &_heap_end - (char *) &_heap_start));
  main();
  while (1) {
  }
}

/* Initial int vectors, in flash */
const __attribute__((
    section(".flash_int_vectors"))) void *stm32_flash_int_vectors[2] = {
    &_stack, stm32_entry};
