/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>

#include <c_types.h>
#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/platforms/esp8266/rboot/rboot/appcode/rboot-api.h"

IRAM NOINSTR void Cache_Read_Enable_New(void) {
  static uint8_t m1 = 0xff, m2;
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
