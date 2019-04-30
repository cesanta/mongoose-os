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

#ifndef CS_MOS_LIBS_ETHERNET_SRC_STM32_STM32_ETH_PHY_H_
#define CS_MOS_LIBS_ETHERNET_SRC_STM32_STM32_ETH_PHY_H_

#include <stdbool.h>

#include "mgos_eth.h"

#include "stm32f7xx_hal_conf.h"
#include "stm32f7xx_hal_eth.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STM32_ETH_PHY_LAN8742A 1

struct stm32_eth_phy_autoneg_result {
  bool complete;
  enum mgos_eth_speed speed;
  enum mgos_eth_duplex duplex;
};

struct stm32_eth_phy_status {
  bool link_up;
  struct mgos_eth_phy_opts opts;
  struct stm32_eth_phy_autoneg_result ar;
};

const char *stm32_eth_phy_name(void);
bool stm32_eth_phy_init(ETH_HandleTypeDef *heth);
bool stm32_eth_phy_start_autoneg(ETH_HandleTypeDef *heth);
bool stm32_eth_phy_set_opts(ETH_HandleTypeDef *heth,
                            const struct mgos_eth_phy_opts *opts);
bool stm32_eth_phy_get_status(ETH_HandleTypeDef *heth,
                              struct stm32_eth_phy_status *status);

#ifdef __cplusplus
}
#endif
#endif /* CS_MOS_LIBS_ETHERNET_SRC_STM32_STM32_ETH_PHY_H_ */
