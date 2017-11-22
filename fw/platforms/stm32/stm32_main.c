/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include <stm32_sdk_hal.h>

#include "common/cs_dbg.h"

#include "mgos_app.h"
#include "mgos_init.h"
#include "mgos_debug.h"
#include "mgos_mongoose.h"
#include "mgos_sys_config.h"
#include "mgos_uart.h"

#include "stm32_fs.h"
#include "stm32_hal.h"
#include "stm32_uart.h"
#include "stm32_lwip.h"

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

static int s_fw_initialized = 0;
static int s_net_initialized = 0;

#define LOOP_DELAY_TICK 10

void mgos_main() {
  GPIO_InitTypeDef info;
  info.Pin = LD1_Pin;
  info.Mode = GPIO_MODE_OUTPUT_PP;
  info.Pull = GPIO_NOPULL;
  info.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD1_GPIO_Port, &info);
  info.Pin = LD2_Pin;
  HAL_GPIO_Init(LD2_GPIO_Port, &info);
  info.Pin = LD3_Pin;
  HAL_GPIO_Init(LD3_GPIO_Port, &info);

  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, 1);

  SystemCoreClockUpdate();

  mgos_app_preinit();

  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);

  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);
  mongoose_init();

  if (mgos_debug_uart_init() != MGOS_INIT_OK) {
    return;
  }

  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }
  LOG(LL_INFO, ("Mongoose OS %s (%s)", mg_build_version, mg_build_id));

  if (stm32_fs_init() != 0) {
    return;
  }

  MX_LWIP_Init();

  if (mgos_init() != MGOS_INIT_OK) {
    return;
  }

  s_fw_initialized = 1;
  LOG(LL_DEBUG, ("Initialization done"));
}

void mgos_loop() {
  if (!s_fw_initialized) {
    return;
  }
  if (!mongoose_poll_scheduled()) {
    HAL_Delay(LOOP_DELAY_TICK);
  }
  if (!s_net_initialized && stm32_have_ip_address()) {
    /* TODO(alashkin): try to replace polling with callbacks */
    stm32_finish_net_init();
    s_net_initialized = 1;
  }

  MX_LWIP_Process();
  mongoose_poll(0);
}
