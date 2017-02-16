#include <stm32_sdk_hal.h>
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_wifi.h"
#include "fw/src/mgos_timers.h"

static int s_mongoose_poll_scheduled;

struct cb_invoke_params {
  mgos_cb_t cb;
  void *arg;
};

int mongoose_poll_scheduled() {
  int ret = s_mongoose_poll_scheduled;
  s_mongoose_poll_scheduled = 0;
  return ret;
}

static void stm32_invoke_cb(void *param) {
  struct cb_invoke_params *params = (struct cb_invoke_params *) param;
  params->cb(params->arg);
  free(params);
}

bool mgos_invoke_cb(mgos_cb_t cb, void *arg) {
  /* Going to call cb from Mongoose context */
  struct cb_invoke_params *params = malloc(sizeof(*params));
  params->cb = cb;
  params->arg = arg;
  mgos_set_timer(0, 0, stm32_invoke_cb, params);
  return true;
}

void mgos_system_restart(int exit_code) {
  (void) exit_code;
  HAL_NVIC_SystemReset();
}

void device_get_mac_address(uint8_t mac[6]) {
  /* TODO(alashkin): implement */
  memset(mac, 0, 6);
}

void mongoose_schedule_poll(void) {
  s_mongoose_poll_scheduled = 0;
}

size_t mgos_get_min_free_heap_size(void) {
  /* TODO(alashkin): implement */
  return 0;
}

size_t mgos_get_free_heap_size(void) {
  /* TODO(alashkin): implement */
  return 0;
}

size_t mgos_get_heap_size(void) {
  /* TODO(alashkin): implement */
  return 0;
}

void mgos_usleep(int usecs) {
  /* STM HAL_Tick has a milliseconds resolution */
  uint32_t msecs = usecs / 1000;
  if (msecs == 0) {
    msecs = 1;
  }
  HAL_Delay(msecs);
  /* TODO(alashkin): try to use RTC timer to get usecs resolution */
}

enum mgos_init_result mgos_sys_config_init_platform(struct sys_config *cfg) {
  return MGOS_INIT_OK;
}

void mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr) {
}

int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  int i = 0;
  (void) ctx;
  do {
    uint32_t rnd;
    if (HAL_RNG_GenerateRandomNumber(&RNG_1, &rnd) != HAL_OK) {
      /* Possible if HAL is locked, fallback to timer */
      rnd = HAL_GetTick();
    }
    int copy_len = len - i;
    if (copy_len > 4) {
      copy_len = 4;
    }
    memcpy(buf + i, &rnd, copy_len);
    i += 4;
  } while (i < len);

  return 0;
}

/*
 * STM32 WDT is too hard to use in mOS;
 * 1. Once enabled it cannot be disabled
 * 2. Its max timeout is 22ms, too small
 * 3. WDT_Refresh() cannot be called at any time,
 *    (if program calls refresh() too often it leads
 *    to exception and reboot)
 * Resume: this WDT is good only for real time programs
 * not for FW with (possibly) arbitrary user code and JS
 * thus, keep it disabled
 */
void mgos_wdt_feed(void) {
}

void mgos_wdt_set_timeout(int secs) {
  (void) secs;
}

void mgos_lock(void) {
}

void mgos_unlock(void) {
}
