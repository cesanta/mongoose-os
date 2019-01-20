#include "mgos.h"
#include "mgos_mongoose.h"
#include "common/cs_dbg.h"
#include "mgos_init_internal.h"
#include "mgos_mongoose_internal.h"
#include "mgos_sys_config.h"
#include "mgos_debug_internal.h"
#include "mgos_mongoose_internal.h"
#include "mgos_uart_internal.h"
#include "mgos_net_hal.h"
#include "ubuntu.h"

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

int main(int argc, char *argv[]) {
  enum mgos_init_result r;

  ubuntu_set_boottime();

  r = mongoose_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR,
        ("mongoose_init=%d (expecting %d), exiting", r, MGOS_INIT_OK));
    return EXIT_FAILURE;
  }
  for (;;) {
    mongoose_poll(1000);
  }
  return EXIT_SUCCESS;

  (void) argc;
  (void) argv;
}

static void dummy_handler(struct mg_connection *nc, int ev, void *ev_data,
                          void *user_data) {
  (void) nc;
  (void) ev;
  (void) ev_data;
  (void) user_data;
}

void mongoose_schedule_poll(bool from_isr) {
  mg_broadcast(mgos_get_mgr(), dummy_handler, NULL, 0);
  (void) from_isr;
}

enum mgos_init_result mongoose_init(void) {
  enum mgos_init_result r;
  int cpu_freq;
  size_t heap_size, free_heap_size;
  struct mgos_net_ip_info ipaddr;
  char ip[INET_ADDRSTRLEN], netmask[INET_ADDRSTRLEN], gateway[INET_ADDRSTRLEN];

  r = mgos_uart_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("Failed to mgos_uart_init: %d", r));
    return r;
  }

  r = mgos_debug_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("Failed to mgos_debug_init: %d", r));
    return r;
  }

  r = mgos_debug_uart_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("Failed to mgos_debug_uart_init: %d", r));
    return r;
  }

  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);

  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);

  LOG(LL_INFO, ("Mongoose OS %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));

  cpu_freq = (int) (mgos_get_cpu_freq() / 1000000);
  heap_size = mgos_get_heap_size();
  free_heap_size = mgos_get_free_heap_size();
  LOG(LL_INFO, ("CPU: %d MHz, heap: %lu total, %lu free", cpu_freq, heap_size,
                free_heap_size));

  mgos_eth_dev_get_ip_info(0, &ipaddr);
  inet_ntop(AF_INET, (void *) &ipaddr.gw.sin_addr, gateway, INET_ADDRSTRLEN);
  inet_ntop(AF_INET, (void *) &ipaddr.ip.sin_addr, ip, INET_ADDRSTRLEN);
  inet_ntop(AF_INET, (void *) &ipaddr.netmask.sin_addr, netmask,
            INET_ADDRSTRLEN);
  LOG(LL_INFO, ("Network: ip=%s netmask=%s gateway=%s", ip, netmask, gateway));

  return mgos_init();
}
