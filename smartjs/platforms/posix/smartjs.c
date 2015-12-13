#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include <sj_hal.h>
#include <sj_v7_ext.h>
#include <string.h>
#include <sj_i2c_js.h>
#include <sj_spi_js.h>
#include <sj_mongoose.h>
#include <sj_mongoose_ws_client.h>
#include <sj_gpio_js.h>
#include <sj_http.h>
#include <sj_uart.h>

#include "smartjs.h"

struct v7 *v7;

void init_smartjs(struct v7 *_v7) {
  v7 = _v7;

  sj_init_v7_ext(v7);

  mongoose_init();
  sj_init_ws_client(v7);
  sj_init_http(v7);
  sj_init_uart(v7);

  init_i2cjs(v7);
  init_spijs(v7);
  init_gpiojs(v7);
}
