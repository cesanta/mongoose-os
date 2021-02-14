/*
 * Copyright 2019 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_net.h"
#include "ubuntu.h"
#include "ubuntu_eth.h"
#include "ubuntu_ipc.h"

void device_get_mac_address(uint8_t mac[6]) {
  char gw_dev[64];
  struct sockaddr_in gw;
  int i;

  if (ubuntu_get_default_gateway(gw_dev, sizeof(gw_dev), &gw)) {
    char buf[100];
    int hex[6];
    int fd;
    int len;
    snprintf(buf, sizeof(buf), "/sys/class/net/%s/address", gw_dev);
    if (!(fd = ubuntu_ipc_open(buf, O_RDONLY))) {
      goto fallback;
    }
    len = read(fd, buf, sizeof(buf));
    close(fd);
    if (len < 17) {
      goto fallback;
    }

    len = sscanf(buf, "%x:%x:%x:%x:%x:%x", &hex[0], &hex[1], &hex[2], &hex[3],
                 &hex[4], &hex[5]);
    if (len != 6) {
      goto fallback;
    }

    for (i = 0; i < 6; i++) {
      mac[i] = (uint8_t) hex[i];
    }
    return;
  }

fallback:
  LOG(LL_WARN,
      ("Cannot find device with default gateway, generating a random MAC"));
  srand(time(NULL));
  for (i = 0; i < 6; i++) {
    mac[i] = (double) rand() / RAND_MAX * 255;
  }
}

void device_set_mac_address(uint8_t mac[6]) {
  // TODO set mac address
  (void) mac;
}
