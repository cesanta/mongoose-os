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

#include "mg_rpc_channel.h"

#ifdef MGOS_HAVE_MONGOOSE
#include "mg_rpc_channel_tcp_common.h"

char *mg_rpc_channel_tcp_get_info(struct mg_connection *c) {
  char buf[100] = {0}, *s = NULL;
  if (c != NULL) {
    mg_conn_addr_to_str(c, buf, sizeof(buf),
                        (MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT |
                         MG_SOCK_STRINGIFY_REMOTE));
    s = strdup(buf);
  }
  return s;
}
#endif /* MGOS_HAVE_MONGOOSE */

bool mg_rpc_channel_true(struct mg_rpc_channel *ch) {
  (void) ch;
  return true;
}

bool mg_rpc_channel_false(struct mg_rpc_channel *ch) {
  (void) ch;
  return false;
}
