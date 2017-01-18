#include <stm32_sdk_hal.h>
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "stm32_spiffs.h"
#include "stm32_uart.h"
#include "stm32_hal.h"
#include "common/cs_dbg.h"

static int s_initialized = 0;
#define LOOP_DELAY_TICK 100

void mgos_main() {
  cs_log_set_level(LL_INFO);

  if (stm32_spiffs_init() != 0) {
    return;
  }
  mongoose_init();
  if (mgos_init() != MGOS_INIT_OK) {
    return;
  }
  s_initialized = 1;
  LOG(LL_DEBUG, ("Initialization done"));
}

void mgos_loop() {
  if (!s_initialized) {
    return;
  }
  if (!mongoose_poll_scheduled()) {
    HAL_Delay(LOOP_DELAY_TICK);
  }
  mongoose_poll(0);
}
