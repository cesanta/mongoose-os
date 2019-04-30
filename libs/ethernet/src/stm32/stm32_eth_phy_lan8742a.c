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

#include "stm32_eth_phy.h"

#include <string.h>

#if STM32_ETH_PHY == STM32_ETH_PHY_LAN8742A

#include "stm32f7xx_hal.h"

#define BIT(n) (1UL << n)

#define LAN8742A_BCR 0x0
#define LAN8742A_BCR_SPEED_100M BIT(13)
#define LAN8742A_BCR_AUTONEG_ON BIT(12)
#define LAN8742A_BCR_AUTONEG_RESTART BIT(9)
#define LAN8742A_BCR_DUPLEX_FULL BIT(8)

#define LAN8742A_BSR 0x1
#define LAN8742A_BSR_AUTONEG_CMPL BIT(5)
#define LAN8742A_BSR_LINK_UP BIT(2)

#define LAN8742A_SCR 0x1f
#define LAN8742A_SCR_AUTONEG_CMPL BIT(12)
#define LAN8742A_SCR_SI_DUPLEX_FULL BIT(4)
#define LAN8742A_SCR_SI_SPEED_100M BIT(3)

#define LAN8742A_CONFIG_DELAY_MS 500

const char *stm32_eth_phy_name(void) {
  return "LAN8742a";
}

bool stm32_eth_phy_init(ETH_HandleTypeDef *heth) {
  (void) heth;
  return true;
}

bool stm32_eth_phy_set_opts(ETH_HandleTypeDef *heth,
                            const struct mgos_eth_phy_opts *opts) {
  uint32_t bcr = 0;
  if (HAL_ETH_ReadPHYRegister(heth, LAN8742A_BCR, &bcr) != HAL_OK) return false;
  uint32_t old_bcr = bcr;

  switch (opts->speed) {
    case MGOS_ETH_SPEED_10M:
      bcr &= ~LAN8742A_BCR_SPEED_100M;
      break;
    case MGOS_ETH_SPEED_100M:
      bcr |= LAN8742A_BCR_SPEED_100M;
      break;
    default:
      return false;
  }

  if (opts->duplex == MGOS_ETH_DUPLEX_FULL) {
    bcr |= LAN8742A_BCR_DUPLEX_FULL;
  } else {
    bcr &= ~LAN8742A_BCR_DUPLEX_FULL;
  }

  if (opts->autoneg_on) {
    bcr |= LAN8742A_BCR_AUTONEG_ON;
  } else {
    bcr &= ~LAN8742A_BCR_AUTONEG_ON;
  }

  if (bcr == old_bcr) return true;

  if (HAL_ETH_WritePHYRegister(heth, LAN8742A_BCR, bcr) != HAL_OK) {
    return false;
  }

  HAL_Delay(LAN8742A_CONFIG_DELAY_MS);

  return true;
}

bool stm32_eth_phy_start_autoneg(ETH_HandleTypeDef *heth) {
  uint32_t bcr = 0;
  if (HAL_ETH_ReadPHYRegister(heth, LAN8742A_BCR, &bcr) != HAL_OK) return false;
  bcr |= LAN8742A_BCR_AUTONEG_RESTART;
  return (HAL_ETH_WritePHYRegister(heth, LAN8742A_BCR, bcr) == HAL_OK);
}

bool stm32_eth_phy_get_status(ETH_HandleTypeDef *heth,
                              struct stm32_eth_phy_status *status) {
  uint32_t bsr = 0, bcr = 0, scr = 0;
  if (HAL_ETH_ReadPHYRegister(heth, LAN8742A_BSR, &bsr) != HAL_OK ||
      HAL_ETH_ReadPHYRegister(heth, LAN8742A_BCR, &bcr) != HAL_OK ||
      HAL_ETH_ReadPHYRegister(heth, LAN8742A_SCR, &scr) != HAL_OK) {
    return false;
  }
  memset(status, 0, sizeof(*status));
  status->link_up = (bsr & LAN8742A_BSR_LINK_UP) != 0;
  struct mgos_eth_phy_opts *opts = &status->opts;
  opts->autoneg_on = (bcr & LAN8742A_BCR_AUTONEG_ON) != 0;
  opts->speed = (bcr & LAN8742A_BCR_SPEED_100M ? MGOS_ETH_SPEED_100M
                                               : MGOS_ETH_SPEED_10M);
  opts->duplex = (bcr & LAN8742A_BCR_DUPLEX_FULL ? MGOS_ETH_DUPLEX_FULL
                                                 : MGOS_ETH_DUPLEX_HALF);
  struct stm32_eth_phy_autoneg_result *ar = &status->ar;
  ar->complete = (bsr & LAN8742A_SCR_AUTONEG_CMPL) != 0;
  ar->speed = (scr & LAN8742A_SCR_SI_SPEED_100M ? MGOS_ETH_SPEED_100M
                                                : MGOS_ETH_SPEED_10M);
  ar->duplex = (scr & LAN8742A_SCR_SI_DUPLEX_FULL ? MGOS_ETH_DUPLEX_FULL
                                                  : MGOS_ETH_DUPLEX_HALF);
  return true;
}

#endif /* STM32_ETH_PHY == STM32_ETH_PHY_LAN8742A */
