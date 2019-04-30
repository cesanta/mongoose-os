/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos_blynk.h"

#include "common/cs_dbg.h"
#include "common/platform.h"

#include "mgos_app.h"
#include "mgos_dlsym.h"
#include "mgos_gpio.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_sys_config.h"
#include "mgos_timers.h"

static struct mg_connection *s_blynk_conn = NULL;
static int s_reconnect_interval_ms = 3000;
static int s_ping_interval_sec = 2;
static int s_read_virtual_pin = 1;
static int s_write_virtual_pin = 2;
static int s_led_pin = 2;

static blynk_handler_t s_blynk_handler = NULL;
static void *s_user_data = NULL;

void blynk_set_handler(blynk_handler_t func, void *user_data) {
  s_blynk_handler = func;
  s_user_data = user_data;
}

static uint16_t getuint16(const uint8_t *buf) {
  return buf[0] << 8 | buf[1];
}

void blynk_send(struct mg_connection *c, uint8_t type, uint16_t id,
                const void *data, uint16_t len) {
  static uint16_t cnt;
  uint8_t header[BLYNK_HEADER_SIZE];

  if (id == 0) id = ++cnt;

  LOG(LL_DEBUG, ("BLYNK SEND type %hhu, id %hu, len %hu", type, id, len));
  header[0] = type;
  header[1] = (id >> 8) & 0xff;
  header[2] = id & 0xff;
  header[3] = (len >> 8) & 0xff;
  header[4] = len & 0xff;

  mg_send(c, header, sizeof(header));
  mg_send(c, data, len);
}

void blynk_virtual_write(struct mg_connection *c, int pin, float val, int id) {
  blynk_printf(c, BLYNK_HARDWARE, id, "vw%c%d%c%f", 0, pin, 0, val);
}

void blynk_printf(struct mg_connection *c, uint8_t type, uint16_t id,
                  const char *fmt, ...) {
  char buf[100];
  int len;
  va_list ap;
  va_start(ap, fmt);
  len = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  blynk_send(c, type, id, buf, len);
}

static void default_blynk_handler(struct mg_connection *c, const char *cmd,
                                  int pin, int val, int id, void *user_data) {
  if (strcmp(cmd, "vr") == 0) {
    if (pin == s_read_virtual_pin) {
      blynk_virtual_write(c, id, s_read_virtual_pin,
                          (float) mgos_get_free_heap_size() / 1024);
    }
  } else if (strcmp(cmd, "vw") == 0) {
    if (pin == s_write_virtual_pin) {
      mgos_gpio_set_mode(s_led_pin, MGOS_GPIO_MODE_OUTPUT);
      mgos_gpio_write(s_led_pin, val);
    }
  }
  (void) user_data;
}

static void handle_blynk_frame(struct mg_connection *c, void *user_data, int id,
                               const uint8_t *data, uint16_t len) {
  LOG(LL_DEBUG, ("BLYNK STATUS: type %hhu, len %hu rlen %hhu", data[0], len,
                 getuint16(data + 3)));
  switch (data[0]) {
    case BLYNK_RESPONSE:
      if (getuint16(data + 3) == 200) {
        LOG(LL_DEBUG, ("BLYNK LOGIN SUCCESS, setting ping timer"));
        mg_set_timer(c, mg_time() + s_ping_interval_sec);
      } else {
        LOG(LL_ERROR, ("BLYNK LOGIN FAILED"));
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
      }
      break;
    case BLYNK_HARDWARE:
      if (len >= 4 && memcmp(data + BLYNK_HEADER_SIZE, "vr", 3) == 0) {
        int i, pin = 0;
        for (i = BLYNK_HEADER_SIZE + 3; i < len; i++) {
          pin *= 10;
          pin += data[i] - '0';
        }
        LOG(LL_DEBUG, ("BLYNK HW: vr %d", pin));
        s_blynk_handler(c, "vr", pin, 0, id, s_user_data);
      } else if (len >= 4 && memcmp(data + BLYNK_HEADER_SIZE, "vw", 3) == 0) {
        int i, pin = 0, val = 0;
        for (i = BLYNK_HEADER_SIZE + 3; i < len && data[i]; i++) {
          pin *= 10;
          pin += data[i] - '0';
        }
        for (i++; i < len && data[i]; i++) {
          val *= 10;
          val += data[i] - '0';
        }
        LOG(LL_DEBUG, ("BLYNK HW: vw %d %d", pin, val));
        s_blynk_handler(c, "vw", pin, val, id, s_user_data);
      }
      break;
  }
  (void) user_data;
}

static void ev_handler(struct mg_connection *c, int ev, void *ev_data,
                       void *user_data) {
  switch (ev) {
    case MG_EV_CONNECT:
      LOG(LL_DEBUG, ("BLYNK CONNECT"));
      blynk_send(c, BLYNK_LOGIN, 1, mgos_sys_config_get_blynk_auth(),
                 strlen(mgos_sys_config_get_blynk_auth()));
      break;
    case MG_EV_RECV:
      while (c->recv_mbuf.len >= BLYNK_HEADER_SIZE) {
        const uint8_t *buf = (const uint8_t *) c->recv_mbuf.buf;
        uint8_t type = buf[0];
        uint16_t id = getuint16(buf + 1), len = getuint16(buf + 3);
        if (id == 0) {
          c->flags |= MG_F_CLOSE_IMMEDIATELY;
          break;
        }
        if (type == BLYNK_RESPONSE) len = 0;
        if (c->recv_mbuf.len < (size_t) BLYNK_HEADER_SIZE + len) break;
        handle_blynk_frame(c, user_data, id, buf, BLYNK_HEADER_SIZE + len);
        mbuf_remove(&c->recv_mbuf, BLYNK_HEADER_SIZE + len);
      }
      break;
    case MG_EV_TIMER:
      blynk_send(c, BLYNK_PING, 0, NULL, 0);
      break;
    case MG_EV_CLOSE:
      LOG(LL_DEBUG, ("BLYNK DISCONNECT"));
      s_blynk_conn = NULL;
      break;
  }
  (void) ev_data;
}

static void reconnect_timer_cb(void *arg) {
  if (!mgos_sys_config_get_blynk_enable() ||
      mgos_sys_config_get_blynk_server() == NULL ||
      mgos_sys_config_get_blynk_auth() == NULL || s_blynk_conn != NULL)
    return;
  s_blynk_conn = mg_connect(mgos_get_mgr(), mgos_sys_config_get_blynk_server(),
                            ev_handler, arg);
}

bool mgos_blynk_init(void) {
  blynk_set_handler(default_blynk_handler, NULL);
  mgos_set_timer(s_reconnect_interval_ms, true, reconnect_timer_cb, NULL);
  return true;
}
