#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_net_hal.h"
#include "mgos_time.h"

static struct timeval s_boottime;

bool ubuntu_set_boottime(void) {
    gettimeofday(&s_boottime, NULL);
      return true;
}

int64_t mgos_uptime_micros(void) {
    struct timeval now;

      gettimeofday(&now, NULL);
        return (int64_t)(now.tv_sec * 1e6 + now.tv_usec - (s_boottime.tv_sec * 1e6 + s_boottime.tv_usec));
}

int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  while (len-- > 0) {
    *buf++ = (unsigned char) rand();
  }
  (void) ctx;
  return 0;
}
