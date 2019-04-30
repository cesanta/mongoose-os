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

#include <stdio.h>
#include <string.h>

#include "mgos_bt.h"
#include "mgos_bt_gap.h"
#include "mgos_bt_gattc.h"
#include "mgos_system.h"

const char *mgos_bt_addr_to_str(const struct mgos_bt_addr *addr, uint32_t flags,
                                char *out) {
  sprintf(out, "%02x:%02x:%02x:%02x:%02x:%02x", addr->addr[0], addr->addr[1],
          addr->addr[2], addr->addr[3], addr->addr[4], addr->addr[5]);
  if (flags & MGOS_BT_ADDR_STRINGIFY_TYPE && (addr->type & 7) != 0) {
    sprintf(out + 17, ",%d", (addr->type & 7));
  }
  return out;
}

bool mgos_bt_addr_from_str(const struct mg_str addr_str,
                           struct mgos_bt_addr *addr) {
  uint8_t *p = addr->addr;
  int at = 0;
  int n = sscanf(addr_str.p, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx,%d", p,
                 p + 1, p + 2, p + 3, p + 4, p + 5, &at);
  addr->type = (enum mgos_bt_addr_type) at;
  return n == 6 || n == 7;
}

int mgos_bt_addr_cmp(const struct mgos_bt_addr *a,
                     const struct mgos_bt_addr *b) {
  return memcmp(a->addr, b->addr, sizeof(b->addr));
}

bool mgos_bt_addr_is_null(const struct mgos_bt_addr *addr) {
  const struct mgos_bt_addr null_addr = {0};
  return (mgos_bt_addr_cmp(addr, &null_addr) == 0);
}

static const char *mgos_bt_uuid128_to_str(const uint8_t *u, char *out) {
  sprintf(out,
          "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
          "%02x%02x%02x%02x%02x%02x",
          u[15], u[14], u[13], u[12], u[11], u[10], u[9], u[8], u[7], u[6],
          u[5], u[4], u[3], u[2], u[1], u[0]);
  return out;
}

const char *mgos_bt_uuid_to_str(const struct mgos_bt_uuid *uuid, char *out) {
  switch (uuid->len) {
    case sizeof(uuid->uuid.uuid16): {
      sprintf(out, "%04x", uuid->uuid.uuid16);
      break;
    }
    case sizeof(uuid->uuid.uuid32): {
      sprintf(out, "%08x", uuid->uuid.uuid32);
      break;
    }
    case sizeof(uuid->uuid.uuid128): {
      mgos_bt_uuid128_to_str(uuid->uuid.uuid128, out);
      break;
    }
    default: {
      sprintf(out, "?(%u)", uuid->len);
    }
  }
  return out;
}

bool mgos_bt_uuid_from_str(const struct mg_str str, struct mgos_bt_uuid *uuid) {
  bool result = false;
  if (str.len == 36) {
    const char *fmt =
        "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-"
        "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx";
    uint8_t *u = uuid->uuid.uuid128;
    if (sscanf(str.p, fmt, &u[15], &u[14], &u[13], &u[12], &u[11], &u[10],
               &u[9], &u[8], &u[7], &u[6], &u[5], &u[4], &u[3], &u[2], &u[1],
               u) == sizeof(uuid->uuid.uuid128)) {
      result = true;
      uuid->len = sizeof(uuid->uuid.uuid128);
    }
  } else if (str.len <= 8) {
    unsigned int u;
    if (sscanf(str.p, "%08x", &u) == 1) {
      result = true;
      if (u & 0xffff0000) {
        uuid->len = sizeof(uuid->uuid.uuid32);
        uuid->uuid.uuid32 = u;
      } else {
        uuid->len = sizeof(uuid->uuid.uuid16);
        uuid->uuid.uuid16 = u;
      }
    }
  }
  return result;
}

int mgos_bt_uuid_cmp(const struct mgos_bt_uuid *a,
                     const struct mgos_bt_uuid *b) {
  int result = 0;
  if (a->len == sizeof(a->uuid.uuid128) || b->len == sizeof(a->uuid.uuid128)) {
    /* 128-bit UUID is always > 16 or 32-bit */
    if (a->len != sizeof(a->uuid.uuid128) &&
        b->len == sizeof(a->uuid.uuid128)) {
      result = -1;
    } else if (a->len == sizeof(a->uuid.uuid128) &&
               b->len != sizeof(a->uuid.uuid128)) {
      result = 1;
    } else {
      for (int i = 15; i >= 0; i--) {
        uint8_t va = a->uuid.uuid128[i];
        uint8_t vb = b->uuid.uuid128[i];
        if (va != vb) {
          result = (va > vb ? 1 : -1);
          break;
        }
      }
    }
  } else {
    uint32_t va, vb;
    va = (a->len == sizeof(a->uuid.uuid16) ? a->uuid.uuid16 : a->uuid.uuid32);
    vb = (b->len == sizeof(a->uuid.uuid16) ? b->uuid.uuid16 : b->uuid.uuid32);
    if (va < vb) {
      result = -1;
    } else if (va > vb) {
      result = 1;
    }
  }
  return result;
}

struct mgos_event_info {
  int ev;
};

static void trigger_cb(void *arg) {
  struct mgos_event_info *ei = arg;
  void *ev_data = ei + 1;
  mgos_event_trigger(ei->ev, ev_data);
  if (ei->ev == MGOS_BT_GATTC_EV_READ_RESULT) {
    struct mgos_bt_gattc_read_result *p = ev_data;
    free((void *) p->data.p);
  } else if (ei->ev == MGOS_BT_GATTC_EV_NOTIFY) {
    struct mgos_bt_gattc_notify_arg *p = ev_data;
    free((void *) p->data.p);
  } else if (ei->ev == MGOS_BT_GAP_EVENT_SCAN_RESULT) {
    struct mgos_bt_gap_scan_result *p = ev_data;
    free((void *) p->adv_data.p);
    free((void *) p->scan_rsp.p);
  }
  free(ei);
}

void mgos_event_trigger_schedule(int ev, const void *ev_data, size_t data_len) {
  struct mgos_event_info *ei = malloc(sizeof(*ei) + data_len);
  ei->ev = ev;
  memcpy(ei + 1, ev_data, data_len);
  if (!mgos_invoke_cb(trigger_cb, ei, false /* from_isr */)) {
    free(ei);
  }
}
