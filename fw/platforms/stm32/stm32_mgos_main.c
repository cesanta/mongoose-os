#include "stm32_hal.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "stm32_spiffs.h"
#include "stm32_uart.h"

void mgos_main() {
  stm32_spiffs_init();
  mongoose_init();
  mgos_init();
}

void mgos_loop() {
  mongoose_poll(10);
}
