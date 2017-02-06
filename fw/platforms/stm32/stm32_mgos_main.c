#include <stm32_sdk_hal.h>
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "stm32_spiffs.h"
#include "stm32_uart.h"
#include "stm32_hal.h"
#include "common/cs_dbg.h"
#include "stm32_lwip.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_uart.h"

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

static int s_fw_initialized = 0;
static int s_net_initialized = 0;

#define LOOP_DELAY_TICK 10

void mgos_main() {
  cs_log_set_level(LL_INFO);

  if (stm32_spiffs_init() != 0) {
    return;
  }

  mongoose_init();

  if (mgos_init_debug_uart(MGOS_DEBUG_UART) != MGOS_INIT_OK) {
    return;
  }

  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }
  LOG(LL_INFO, ("Mongoose OS Firmware %s (%s)", mg_build_version, mg_build_id));

  mgos_app_preinit();

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

  mongoose_poll(0);
}
