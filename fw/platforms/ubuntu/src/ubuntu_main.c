#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "mgos.h"
#include "mgos_mongoose.h"
#include "common/cs_dbg.h"
#include "mgos_init.h"
#include "mgos_mongoose_internal.h"
#include "mgos_sys_config.h"

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  mongoose_init();
  for (;;) {
    mongoose_poll(1000);
  }
  return EXIT_SUCCESS;
}

static void dummy_handler(struct mg_connection *nc, int ev, void *ev_data,
                          void *user_data) {
  (void) nc;
  (void) ev;
  (void) ev_data;
  (void) user_data;
}

void mongoose_schedule_poll(bool from_isr) {
  (void) from_isr;
  mg_broadcast(mgos_get_mgr(), dummy_handler, NULL, 0);
}

enum mgos_init_result mgos_sys_config_init_platform(struct mgos_config *cfg) {
  cs_log_set_level(cfg->debug.level);
  return MGOS_INIT_OK;
}

void device_get_mac_address(uint8_t mac[6]) {
  int i;
  srand(time(NULL));
  for (i = 0; i < 6; i++) {
    mac[i] = (double) rand() / RAND_MAX * 255;
  }
}
