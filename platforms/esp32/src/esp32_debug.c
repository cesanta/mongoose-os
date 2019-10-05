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

#include "esp32_debug.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/reent.h>

#include "freertos/FreeRTOS.h"

#include "esp_vfs.h"

#include <lwip/pbuf.h>
#include <lwip/tcpip.h>
#include <lwip/udp.h>

#include "common/cs_dbg.h"
#include "common/mbuf.h"
#include "mgos_debug.h"
#include "mgos_debug_hal.h"
#include "mgos_features.h"
#include "mgos_hal.h"
#include "mgos_init.h"

static int debug_open(const char *path, int flags, int mode) {
  if (strcmp(path, "/in") == 0) {
    return 0;
  } else if (strcmp(path, "/out") == 0) {
    return 1;
  } else if (strcmp(path, "/err") == 0) {
    return 2;
  }
  (void) flags;
  (void) mode;
  return -1;
}

static ssize_t debug_read(int fd, void *dst, size_t size) {
  errno = EBADF;
  return -1;
}

static ssize_t debug_write(int fd, const void *data, size_t size) {
  if (fd == 1 || fd == 2) {
    mgos_debug_write(fd, data, size);
  } else {
    errno = EBADF;
    return -1;
  }
  return size;
}

enum mgos_init_result esp32_debug_init() {
  esp_vfs_t vfs = {
      .open = &debug_open,
      .read = &debug_read,
      .write = &debug_write,
  };
  if (esp_vfs_register("/__mgos_debug", &vfs, NULL) != ESP_OK) {
    return MGOS_INIT_CONSOLE_INIT_FAILED;
  }
  /*
   * Now the tricky part: poke our own FDs inside existing std{in,out,err} FILE
   * structs. We do not reallocate them because pointers have now been copied to
   * all the existing RTOS tasks.
   */
  _GLOBAL_REENT->_stdin->_file = open("/__mgos_debug/in", O_RDONLY);
  _GLOBAL_REENT->_stdout->_file = open("/__mgos_debug/out", O_WRONLY);
  _GLOBAL_REENT->_stderr->_file = open("/__mgos_debug/err", O_WRONLY);
  return MGOS_INIT_OK;
}

#if MGOS_ENABLE_DEBUG_UDP
static struct udp_pcb *s_upcb = NULL;
static ip_addr_t s_dst;
static uint16_t s_port;

enum mgos_init_result mgos_debug_udp_init(const char *dst) {
  uint32_t ip1, ip2, ip3, ip4, port;
  if (sscanf(dst, "%u.%u.%u.%u:%u", &ip1, &ip2, &ip3, &ip4, &port) != 5) {
    LOG(LL_ERROR, ("Invalid address"));
    return MGOS_INIT_DEBUG_INIT_FAILED;
  }
  IP_ADDR4(&s_dst, ip1, ip2, ip3, ip4);
  s_port = port;
  struct udp_pcb *upcb = udp_new();
  if (upcb == NULL || udp_bind(upcb, IP_ADDR_ANY, 0 /* any port */) != ERR_OK) {
    return MGOS_INIT_DEBUG_INIT_FAILED;
  }
  if (s_upcb != NULL) udp_remove(s_upcb);
  s_upcb = upcb;
  LOG(LL_INFO, ("UDP log set up to %s", dst));
  return MGOS_INIT_OK;
}

void udp_flush_cb(void *arg) {
  struct pbuf *p = (struct pbuf *) arg;
  udp_sendto(s_upcb, p, &s_dst, s_port);
  pbuf_free(p);
}

void mgos_debug_udp_send(const struct mg_str prefix, const struct mg_str data) {
  if (s_upcb == NULL) return;
  struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, prefix.len + data.len, PBUF_RAM);
  if (p == NULL) return;
  memcpy(p->payload, prefix.p, prefix.len);
  memcpy((char *) p->payload + prefix.len, data.p, data.len);
  if (tcpip_callback_with_block(udp_flush_cb, p, false /* block */) != ERR_OK) {
    pbuf_free(p);
  }
}
#endif /* MGOS_ENABLE_DEBUG_UDP */
