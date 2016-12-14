/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mbed.h"
#include "fw/src/miot_init.h"
#include "fw/src/miot_mongoose.h"
#include "stm32_spiffs.h"

int main() {
  miot_stm32_init_spiffs_init();
  mongoose_init();
  mg_init();

  while(1) {
    mongoose_poll(100);
  }

  return 0;
}
