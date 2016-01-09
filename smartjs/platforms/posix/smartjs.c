#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "smartjs/src/sj_hal.h"
#include "smartjs/src/sj_v7_ext.h"
#include <string.h>
#include "smartjs/src/sj_i2c_js.h"
#include "smartjs/src/sj_spi_js.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/sj_mongoose_ws_client.h"
#include "smartjs/src/sj_gpio_js.h"
#include "smartjs/src/sj_http.h"
#include "smartjs/src/sj_uart.h"
#include "smartjs/src/sj_clubby.h"

#include "smartjs.h"

struct v7 *v7;

void init_smartjs(struct v7 *_v7) {
  v7 = _v7;

  sj_init_v7_ext(v7);

  mongoose_init();
  sj_init_ws_client(v7);
  sj_init_http(v7);
  sj_init_uart(v7);
  sj_init_clubby(v7);

  init_i2cjs(v7);
  init_spijs(v7);
  init_gpiojs(v7);
}
