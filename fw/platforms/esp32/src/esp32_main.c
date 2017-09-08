/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "esp_attr.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "rom/ets_sys.h"
#include "rom/spi_flash.h"

#include "common/cs_dbg.h"
#include "mgos_app.h"
#include "mgos_debug.h"
#include "mgos_hal.h"
#include "mgos_hooks.h"
#include "mgos_init.h"
#include "mgos_mongoose.h"
#include "mgos_net_hal.h"
#include "mgos_sys_config.h"
#include "mgos_uart.h"
#include "mgos_updater_common.h"
#include "mgos_vfs.h"
#ifdef MGOS_HAVE_WIFI
#include "esp32_wifi.h"
#endif

#include "fw/platforms/esp32/src/esp32_debug.h"
#include "fw/platforms/esp32/src/esp32_exc.h"
#include "fw/platforms/esp32/src/esp32_fs.h"
#include "fw/platforms/esp32/src/esp32_updater.h"

#ifndef MGOS_TASK_STACK_SIZE
#define MGOS_TASK_STACK_SIZE 8192 /* in bytes */
#endif

#ifndef MGOS_TASK_PRIORITY
#define MGOS_TASK_PRIORITY 5
#endif

#ifndef MGOS_TASK_QUEUE_LENGTH
#define MGOS_TASK_QUEUE_LENGTH 32
#endif

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

esp_err_t esp32_wifi_ev(system_event_t *event);

esp_err_t event_handler(void *ctx, system_event_t *event) {
  switch (event->event_id) {
#ifdef MGOS_HAVE_WIFI
    case SYSTEM_EVENT_STA_START:
    case SYSTEM_EVENT_STA_STOP:
    case SYSTEM_EVENT_STA_GOT_IP:
    case SYSTEM_EVENT_STA_CONNECTED:
    case SYSTEM_EVENT_STA_DISCONNECTED:
    case SYSTEM_EVENT_AP_STACONNECTED:
    case SYSTEM_EVENT_AP_STADISCONNECTED:
    case SYSTEM_EVENT_SCAN_DONE:
      return esp32_wifi_ev(event);
      break;
#endif
#ifdef MGOS_HAVE_ETHERNET
    case SYSTEM_EVENT_ETH_START:
    case SYSTEM_EVENT_ETH_STOP:
      break;
    case SYSTEM_EVENT_ETH_CONNECTED: {
      mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, 0,
                            MGOS_NET_EV_CONNECTED);
      break;
    }
    case SYSTEM_EVENT_ETH_DISCONNECTED: {
      mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, 0,
                            MGOS_NET_EV_DISCONNECTED);
      break;
    }
    case SYSTEM_EVENT_ETH_GOT_IP: {
      mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, 0,
                            MGOS_NET_EV_IP_ACQUIRED);
      break;
    }
#endif
    default:
      LOG(LL_INFO, ("event: %d", event->event_id));
  }
  return ESP_OK;
}

struct mgos_event {
  mgos_cb_t cb;
  void *arg;
};

static void s_init_done_hook(enum mgos_hook_type type,
                             const struct mgos_hook_arg *arg, void *userdata) {
  /* initialize TZ env variable with the sys.tz_spec config value */
  char *tz_spec = get_cfg()->sys.tz_spec;
  if (tz_spec == NULL) {
    tz_spec = "";
  }

  setenv("TZ", tz_spec, 1);
  tzset();

  (void) type;
  (void) arg;
  (void) userdata;
}

static enum mgos_init_result esp32_mgos_init() {
  enum mgos_init_result r;

  /* Enable WDT for this task. It will be fed by Mongoose polling loop. */
  esp_task_wdt_feed();

#if MGOS_ENABLE_UPDATER
  esp32_updater_early_init();
#endif

  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);
  mongoose_init();

  r = esp32_debug_init();
  if (r != MGOS_INIT_OK) return r;
  r = mgos_debug_uart_init();
  if (r != MGOS_INIT_OK) return r;

  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }
  LOG(LL_INFO, ("Mongoose OS %s (%s)%s", mg_build_version, mg_build_id,
#if MGOS_ENABLE_UPDATER
                (esp32_is_first_boot() ? ", first boot" : "")
#else
                ""
#endif
                    ));
  LOG(LL_INFO, ("ESP-IDF %s", esp_get_idf_version()));
  LOG(LL_INFO,
      ("Boot partition: %s; flash: %uM; RAM: %u total, %u free",
       esp_ota_get_boot_partition()->label, g_rom_flashchip.chip_size / 1048576,
       mgos_get_heap_size(), mgos_get_free_heap_size()));

  if (!esp32_fs_init()) {
    LOG(LL_ERROR, ("Failed to mount FS"));
    return MGOS_INIT_FS_INIT_FAILED;
  }

  mgos_wdt_feed();

#if MGOS_ENABLE_UPDATER
  if (esp32_is_first_boot() && mgos_upd_apply_update() < 0) {
    return MGOS_INIT_APPLY_UPDATE_FAILED;
  }
#endif

#ifdef CS_MMAP
  mgos_vfs_mmap_init();
#endif

  esp32_exception_handler_init();

  /*
   * We also need to initialize TZ env variable with the sys.tz_spec config
   * value, but we can't do that here because sys_config is not yet
   * initialized, so we register the INIT_DONE hook.
   */
  mgos_hook_register(MGOS_HOOK_INIT_DONE, s_init_done_hook, NULL);

  if ((r = mgos_init()) != MGOS_INIT_OK) return r;

  return MGOS_INIT_OK;
}

extern SemaphoreHandle_t s_mgos_mux;
static QueueHandle_t s_main_queue;
/* Note: we cannot use mutex here because there is no recursive mutex
 * that can be used from ISR as well as from task. mgos_invoke_cb pust an item
 * on the queue and may cause a context switch and re-enter schedule_poll.
 * Hence this elaborate dance we perform with poll counter. */
static volatile uint32_t s_mg_last_poll = 0;
static volatile bool s_mg_poll_scheduled = false;
static volatile bool s_mg_want_poll = false;
static portMUX_TYPE s_poll_spinlock = portMUX_INITIALIZER_UNLOCKED;
static TimerHandle_t s_mg_poll_timer;

static void IRAM_ATTR mgos_mg_poll_cb(void *arg) {
  uint32_t timeout_ms, timeout_ticks, n = 0;
  portENTER_CRITICAL(&s_poll_spinlock);
  do {
    portEXIT_CRITICAL(&s_poll_spinlock);
    s_mg_want_poll = false;
    mongoose_poll(0);
    timeout_ms = mg_lwip_get_poll_delay_ms(mgos_get_mgr());
    portENTER_CRITICAL(&s_poll_spinlock);
    if (timeout_ms > 100) timeout_ms = 100;
    timeout_ticks = timeout_ms / portTICK_PERIOD_MS;
    n++;
  } while (n < 10 && (s_mg_want_poll || timeout_ticks == 0));
  s_mg_poll_scheduled = false;
  s_mg_last_poll++;
  portEXIT_CRITICAL(&s_poll_spinlock);
  if (!s_mg_want_poll && timeout_ticks > 0) {
    xTimerChangePeriod(s_mg_poll_timer, timeout_ticks, 10);
    xTimerReset(s_mg_poll_timer, 10);
  } else {
    mongoose_schedule_poll(false /* from_isr */);
  }
  (void) arg;
}

void IRAM_ATTR mongoose_schedule_poll(bool from_isr) {
  /* Prevent piling up of poll callbacks. */
  portENTER_CRITICAL(&s_poll_spinlock);
  s_mg_want_poll = true;
  if (!s_mg_poll_scheduled) {
    uint32_t last_poll = s_mg_last_poll;
    portEXIT_CRITICAL(&s_poll_spinlock);
    if (mgos_invoke_cb(mgos_mg_poll_cb, NULL, from_isr)) {
      portENTER_CRITICAL(&s_poll_spinlock);
      if (s_mg_last_poll == last_poll) {
        s_mg_poll_scheduled = true;
      }
      portEXIT_CRITICAL(&s_poll_spinlock);
    }
  } else {
    portEXIT_CRITICAL(&s_poll_spinlock);
  }
}

void mgos_mg_poll_timer_cb(TimerHandle_t t) {
  mongoose_schedule_poll(false /* from_isr */);
  (void) t;
}

void mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr) {
  (void) mgr;
  mongoose_schedule_poll(false /* from_isr */);
}

void mgos_task(void *arg) {
  struct mgos_event e;
  s_main_queue = xQueueCreate(MGOS_TASK_QUEUE_LENGTH, sizeof(e));
  srand(esp_random()); /* esp_random() uses HW RNG */

  mgos_app_preinit();

  enum mgos_init_result r = esp32_mgos_init();
  bool success = (r == MGOS_INIT_OK);
  if (!success) LOG(LL_ERROR, ("MGOS init failed: %d", r));

#if MGOS_ENABLE_UPDATER
  mgos_upd_boot_finish(success, esp32_is_first_boot());
#endif

  if (!success) {
    /* Arbitrary delay to make potential reboot loop less tight. */
    mgos_usleep(500000);
    mgos_system_restart(0);
  }

  while (true) {
    while (xQueueReceive(s_main_queue, &e, 10 /* tick */)) {
      e.cb(e.arg);
    }
  }

  (void) arg;
}

bool IRAM_ATTR mgos_invoke_cb(mgos_cb_t cb, void *arg, bool from_isr) {
  struct mgos_event e = {.cb = cb, .arg = arg};
  if (from_isr) {
    int should_yield = false;
    if (!xQueueSendToBackFromISR(s_main_queue, &e, &should_yield)) {
      return false;
    }
    if (should_yield) {
      portYIELD_FROM_ISR();
    }
    return true;
  } else {
    return xQueueSendToBack(s_main_queue, &e, 10);
  }
}

/*
 * Note that this function may be invoked from a very low level.
 * This is where ESP_EARLY_LOG prints (via ets_printf).
 */
static IRAM void sdk_putc(char c) {
  if (mgos_debug_uart_is_suspended()) return;
  ets_write_char_uart(c);
}

extern enum cs_log_level cs_log_cur_msg_level;
static int sdk_debug_vprintf(const char *fmt, va_list ap) {
  /* Do not log SDK messages anywhere except UART. */
  cs_log_cur_msg_level = LL_VERBOSE_DEBUG;
  int res = vprintf(fmt, ap);
  cs_log_cur_msg_level = LL_NONE;
  return res;
}

void app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();
  mgos_uart_init();
  mgos_debug_init();
  ets_install_putc1(sdk_putc);
  ets_install_putc2(NULL);
  esp_log_set_vprintf(sdk_debug_vprintf);
  s_mg_poll_timer = xTimerCreate("mg_poll", 10, pdFALSE /* reload */, 0,
                                 mgos_mg_poll_timer_cb);
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  s_mgos_mux = xSemaphoreCreateRecursiveMutex();
  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);
  xTaskCreate(mgos_task, "mgos", MGOS_TASK_STACK_SIZE, NULL, MGOS_TASK_PRIORITY,
              NULL);
}
