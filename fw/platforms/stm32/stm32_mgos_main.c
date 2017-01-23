#include <stm32_sdk_hal.h>
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "stm32_spiffs.h"
#include "stm32_uart.h"
#include "stm32_hal.h"
#include "common/cs_dbg.h"
#include "stm32_lwip.h"

static int s_fw_initialized = 0;
static int s_net_initialized = 0;

#define LOOP_DELAY_TICK 10

void mgos_main() {
  cs_log_set_level(LL_INFO);

  if (stm32_spiffs_init() != 0) {
    return;
  }
  mongoose_init();
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
