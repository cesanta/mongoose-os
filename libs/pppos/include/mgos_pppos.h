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

#pragma once

#include <stdbool.h>

#include "common/mg_str.h"

#include "mgos_event.h"
#include "mgos_net.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Retrieve IP configuration of the provided instance number (which should be
 * of type `MGOS_NET_IF_TYPE_PPP`), and fill provided `ip_info` with it. Returns
 * `true` in case of success, false otherwise.
 */
bool mgos_pppos_dev_get_ip_info(int if_instance,
                                struct mgos_net_ip_info *ip_info);

#define MGOS_PPPOS_BASE MGOS_EVENT_BASE('P', 'o', 'S')

enum mgos_pppos_event {
  MGOS_PPPOS_CMD_RESP = MGOS_PPPOS_BASE,  // ev_data: struct mgos_pppos_cmd_resp
  MGOS_PPPOS_INIT,                        // ev_data: NULL
  MGOS_PPPOS_INFO,                        // ev_data: struct mgos_ppos_info_arg
};

struct mgos_pppos_info_arg {
  struct mg_str info; /* ATI command response */
  struct mg_str imei;
  struct mg_str imsi;
  struct mg_str iccid;
};

/* Create PPPoS interface instance.
 * cfg must remain valid for the lifetime of the instance. */
bool mgos_pppos_create(const struct mgos_config_pppos *cfg, int if_instance);

/* Initiate connection. */
bool mgos_pppos_connect(int if_instance);

/* Disconnect interface. */
bool mgos_pppos_disconnect(int if_instance);

/*
 * Command/sequence callback.
 * ok will be set to the command response status (OK = true, ERROR == false).
 * data will contain response payload - any data received from the modem
 * after the comamnd was sent and before OK/ERROR was received.
 * Return value should indicate whether sequence should continue: if callback
 * returns true, the sequence continued and next command is executed.
 * If the return value is false, the sequence is aborted, remaining commands
 * (if any) are not executed and sequence finalizer is run (if provided).
 */
typedef bool (*mgos_pppos_cmd_cb_t)(void *cb_arg, bool ok, struct mg_str data);

struct mgos_pppos_cmd {
  const char *cmd;
  mgos_pppos_cmd_cb_t cb;
  void *cb_arg;
};

/*
 * Send custom command sequence. This can be sent both when the interface is
 * idle as well as when connection is up. In the latter case the connectio will
 * be suspended to execute the command sequence.
 * If cb is set, it will receive command response (not including  OK/ERROR).
 * The sequence must end with an entry that has cmd set to NULL.
 * Callback on this last entry, if set, serves as a finalzier: it will be
 * invoked when the sequence ends, successfully or not (this is passed as "ok"
 * argument). The finalizer does not receive and data.
 *
 * mgos_pppos_run_cmds may return false if the modem is busy executing
 * another command sequence.
 *
 * Example:
 *   // Retrieve GNSS status from SimCom GPS-capable modem:
 *   const struct mgos_pppos_cmd cmds[] = {
 *       {.cmd = "AT+CGNSPWR=1"},  // Turn on GNSS (if not already).
 *       {.cmd = "AT+CGNSINF", .cb = gnsinf_cb},
 *       {.cmd = NULL},
 *   };
 *   mgos_pppos_run_cmds(0, cmds);
 *
 *   bool gnsinf_cb(void *cb_arg, bool ok, struct mg_str data) {
 *     if (!ok) return false;
 *     int run = 0, fix = 0;
 *     char time[20];
 *     float lat = 0, lon = 0, alt = 0, speed = 0, course = 0;
 *     sscanf(data.p, "+CGNSINF: %d,%d,%18s,%f,%f,%f,%f,%f",
 *                    &run, &fix, time, &lat, &lon, &alt, &speed, &course);
 *     if (fix) {
 *       LOG(LL_INFO, ("lat,lon: %f,%f alt: %.3f speed: %.2f course: %.2f",
 *                     lat, lon, alt, speed, course));
 *     } else {
 *       LOG(LL_INFO, ("No GNSS fix yet"));
 *     }
 *     (void) cb_arg;
 *     return true;
 *   }
 */
bool mgos_pppos_run_cmds(int if_instance, const struct mgos_pppos_cmd *cmds);

struct mg_str mgos_pppos_get_imei(int if_instance);
struct mg_str mgos_pppos_get_imsi(int if_instance);
struct mg_str mgos_pppos_get_iccid(int if_instance);

#ifdef __cplusplus
}
#endif
