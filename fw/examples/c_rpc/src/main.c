/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/platform.h"
#include "frozen/frozen.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_rpc.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_wifi.h"

#if CS_PLATFORM == CS_P_ESP8266
/* On ESP-12E there is a blue LED connected to GPIO2 (aka U1TX). */
#define LED_GPIO 2
#define BUTTON_GPIO 0 /* Usually a "Flash" button. */
#define BUTTON_PULL MGOS_GPIO_PULL_UP
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#elif CS_PLATFORM == CS_P_ESP32
/* Unfortunately, there is no LED on DevKitC, so this is random GPIO. */
#define LED_GPIO 17
#define BUTTON_GPIO 0 /* Usually a "Flash" button. */
#define BUTTON_PULL MGOS_GPIO_PULL_UP
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#elif CS_PLATFORM == CS_P_CC3200
/* On CC3200 LAUNCHXL pin 64 is the red LED. */
#define LED_GPIO 64                     /* The red LED on LAUNCHXL */
#define BUTTON_GPIO 15                  /* SW2 on LAUNCHXL */
#define BUTTON_PULL MGOS_GPIO_PULL_NONE /* External pull-downs */
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_NEG
#elif(CS_PLATFORM == CS_P_STM32) && defined(BSP_NUCLEO_F746ZG)
/* Nucleo-144 F746 */
#define LED_GPIO STM32_PIN_PB7     /* Blue LED */
#define BUTTON_GPIO STM32_PIN_PC13 /* Blue user button */
#define BUTTON_PULL MGOS_GPIO_PULL_NONE
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#elif(CS_PLATFORM == CS_P_STM32) && defined(BSP_DISCO_F746G)
/* Discovery-0 F746 */
#define LED_GPIO STM32_PIN_PI1     /* Green LED */
#define BUTTON_GPIO STM32_PIN_PI11 /* Blue user button */
#define BUTTON_PULL MGOS_GPIO_PULL_NONE
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#else
#error Unknown platform
#endif

static void inc_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                        struct mg_rpc_frame_info *fi, struct mg_str args) {
  struct mbuf fb;
  struct json_out out = JSON_OUT_MBUF(&fb);

  mbuf_init(&fb, 20);

  int num = 0;
  if (json_scanf(args.p, args.len, ri->args_fmt, &num) == 1) {
    json_printf(&out, "{num: %d}", num + 1);
  } else {
    json_printf(&out, "{error: %Q}", "num is required");
  }

  mgos_gpio_toggle(LED_GPIO);

  printf("%d + 1 = %d\n", num, num + 1);

  mg_rpc_send_responsef(ri, "%.*s", fb.len, fb.buf);
  ri = NULL;

  mbuf_free(&fb);

  (void) cb_arg;
  (void) fi;
}

int peer_num = 111;

static void rpc_resp(struct mg_rpc *c, void *cb_arg,
                     struct mg_rpc_frame_info *fi, struct mg_str result,
                     int error_code, struct mg_str error_msg) {
  if (error_code == 0 && result.len > 0) {
    LOG(LL_INFO, ("response: %.*s", (int) result.len, result.p));
    json_scanf(result.p, result.len, "{num: %d}", &peer_num);
  }

  (void) c;
  (void) cb_arg;
  (void) fi;
  (void) error_msg;
}

static void call_specific_peer(const char *peer) {
  struct mg_rpc *c = mgos_rpc_get_global();
  struct mg_rpc_call_opts opts;
  LOG(LL_INFO, ("Calling %s", peer));
  memset(&opts, 0, sizeof(opts));
  opts.dst = mg_mk_str(peer);
  mg_rpc_callf(c, mg_mk_str("Example.Increment"), rpc_resp, NULL, &opts,
               "{num: %d}", peer_num);
}

static void call_peer(void) {
  const char *peer = get_cfg()->c_rpc.peer;
  if (peer == NULL) {
    LOG(LL_INFO, ("Peer address not configured, set c_rpc.peer (e.g. ws://192.168.1.234/rpc)"));
    return;
  }
  call_specific_peer(peer);
}

void wifi_changed(enum mgos_wifi_status ev, void *arg) {
  if (ev != MGOS_WIFI_IP_ACQUIRED) return;
  call_peer();
  (void) arg;
}

static void button_cb(int pin, void *arg) {
  LOG(LL_INFO, ("Click!"));
  call_peer();
  (void) pin;
  (void) arg;
}

static void call_peer_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                        struct mg_rpc_frame_info *fi, struct mg_str args) {
  char *peer = NULL;
  if (json_scanf(args.p, args.len, ri->args_fmt, &peer) == 1) {
    call_specific_peer(peer);
    free(peer);
  } else {
    call_peer();
  }
  mg_rpc_send_responsef(ri, NULL);

  (void) cb_arg;
  (void) fi;
}

enum mgos_app_init_result mgos_app_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, "Example.Increment", "{num: %d}", inc_handler, NULL);
  mg_rpc_add_handler(c, "Example.CallPeer", "{peer: %Q}", call_peer_handler, NULL);
  mgos_wifi_add_on_change_cb(wifi_changed, NULL);
  mgos_gpio_set_mode(LED_GPIO, MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_set_button_handler(BUTTON_GPIO, BUTTON_PULL, BUTTON_EDGE,
                               50 /* debounce_ms */, button_cb, NULL);
  return MGOS_APP_INIT_SUCCESS;
}
