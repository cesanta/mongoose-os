#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_net_hal.h"

int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  while (len-- > 0) {
    *buf++ = (unsigned char)rand();
  }
  (void)ctx;
  return 0;
}
