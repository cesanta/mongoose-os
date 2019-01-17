#include "mgos.h"
#include "mgos_mongoose.h"
#include "common/cs_dbg.h"
#include "mgos_init_internal.h"
#include "mgos_mongoose_internal.h"
#include "mgos_sys_config.h"
#include "mgos_debug_internal.h"
#include "mgos_mongoose_internal.h"
#include "mgos_uart_internal.h"

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

int main(int argc, char *argv[]) {
  enum mgos_init_result r;

  r = mongoose_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("mongoose_init=%d (expecting %d), exiting", r, MGOS_INIT_OK));
    return EXIT_FAILURE;
  }
  for (;;) {
    mongoose_poll(1000);
  }
  return EXIT_SUCCESS;

  (void)argc;
  (void)argv;
}

static void dummy_handler(struct mg_connection *nc, int ev, void *ev_data,
                          void *user_data) {
  (void)nc;
  (void)ev;
  (void)ev_data;
  (void)user_data;
}

void mongoose_schedule_poll(bool from_isr) {
  mg_broadcast(mgos_get_mgr(), dummy_handler, NULL, 0);
  (void)from_isr;
}

void device_get_mac_address(uint8_t mac[6]) {
  int i;

  srand(time(NULL));
  for (i = 0; i < 6; i++) {
    mac[i] = (double)rand() / RAND_MAX * 255;
  }
}

enum mgos_init_result mongoose_init(void) {
  enum mgos_init_result r;
  int    cpu_freq;
  size_t heap_size, free_heap_size;

  r = mgos_uart_init();
  LOG(LL_ERROR, ("mgos_uart_init=%d", r));
  if (r != MGOS_INIT_OK) {
    return r;
  }

  r = mgos_debug_init();
  LOG(LL_ERROR, ("mgos_debug_init=%d", r));
  if (r != MGOS_INIT_OK) {
    return r;
  }

  r = mgos_debug_uart_init();
  LOG(LL_ERROR, ("mgos_debug_uart_init=%d", r));
  if (r != MGOS_INIT_OK) {
    return r;
  }

  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);

  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);

  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }

  cpu_freq       = (int)(mgos_get_cpu_freq() / 1000000);
  heap_size      = mgos_get_heap_size();
  free_heap_size = mgos_get_free_heap_size();
  LOG(LL_INFO, ("Mongoose OS %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("CPU: %d MHz, heap: %lu total, %lu free", cpu_freq, heap_size, free_heap_size));

  return mgos_init();
}
