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

#include <c_types.h>
#include "esp_missing_includes.h"
#include "platforms/esp8266/rboot/rboot/appcode/rboot-api.h"

uint32_t esp_stack_canary_en = 0;

IRAM NOINSTR void Cache_Read_Enable_New(void) {
  static uint8_t m1 = 0xff, m2 = 0xff;
  if (m1 == 0xff) {
    uint32_t addr;
    rboot_config conf;

    Cache_Read_Disable();

    SPIRead(BOOT_CONFIG_SECTOR * SECTOR_SIZE, &conf, sizeof(conf));

    addr = conf.roms[conf.current_rom];
    addr /= 0x100000;

    m1 = addr % 2;
    m2 = addr / 2;
  }

  Cache_Read_Enable(m1, m2, 1);
}
