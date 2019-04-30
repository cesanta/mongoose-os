/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#include "stm32_spi_master_internal.h"

#include "common/cs_dbg.h"

#include "mgos_gpio.h"

bool stm32_gspi_configure(struct mgos_spi *c,
                          const struct mgos_config_spi *cfg) {
  /* Reset everything and disable. Enable manual SS control. */
  uint32_t cr1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
  uint32_t cr2 = 0;
#ifdef SPI_CR2_DS_Pos
  /* Configure 8-bit transfers. */
  cr2 |= SPI_CR2_FRXTH | (7 << SPI_CR2_DS_Pos);
#endif
  c->regs->CR1 = cr1;
  c->regs->CR2 = cr2;
  c->debug = cfg->debug;
  return true;
}

bool stm32_gspi_set_freq(struct mgos_spi *c, int freq) {
  CLEAR_BIT(c->regs->CR1, SPI_CR1_SPE);
  int eff_freq;
  switch (c->unit_no) {
    case 1:
    case 4:
    case 5:
    case 6:
      eff_freq = HAL_RCC_GetPCLK2Freq();
      break;
    case 2:
    case 3:
      eff_freq = HAL_RCC_GetPCLK1Freq();
      break;
    default:
      return false;
  }
  uint32_t br = 0;
  eff_freq /= 2;
  while (eff_freq > freq) {
    br++;
    eff_freq /= 2;
    if (br > 7) return false;
  }
  MODIFY_REG(c->regs->CR1, SPI_CR1_BR_Msk, br << SPI_CR1_BR_Pos);
  c->freq = freq;
  if (c->debug) {
    LOG(LL_DEBUG, ("freq %d => div %d (br %d) => eff_freq %d", c->freq,
                   (int) (1 << (br + 1)), (int) br, eff_freq));
  }
  return true;
}

bool stm32_gspi_set_mode(struct mgos_spi *c, int mode) {
  CLEAR_BIT(c->regs->CR1, SPI_CR1_CPOL | SPI_CR1_CPHA);
  switch (mode) {
    case 0:
      mgos_gpio_set_pull(c->sclk_gpio, MGOS_GPIO_PULL_DOWN);
      break;
    case 1:
      mgos_gpio_set_pull(c->sclk_gpio, MGOS_GPIO_PULL_DOWN);
      SET_BIT(c->regs->CR1, SPI_CR1_CPHA);
      break;
    case 2:
      mgos_gpio_set_pull(c->sclk_gpio, MGOS_GPIO_PULL_UP);
      SET_BIT(c->regs->CR1, SPI_CR1_CPOL);
      break;
    case 3:
      mgos_gpio_set_pull(c->sclk_gpio, MGOS_GPIO_PULL_UP);
      SET_BIT(c->regs->CR1, SPI_CR1_CPOL | SPI_CR1_CPHA);
      break;
    default:
      return false;
  }
  return true;
}

inline static void stm32_gspi_wait_tx_empty(struct mgos_spi *c) {
  while (!(c->regs->SR & SPI_SR_TXE)) {
  }
}

bool stm32_gspi_run_txn_fd(struct mgos_spi *c, const struct mgos_spi_txn *txn) {
  size_t len = txn->fd.len;
  const uint8_t *tx_data = (const uint8_t *) txn->hd.tx_data;
  uint8_t *rx_data = (uint8_t *) txn->fd.rx_data;
  volatile uint8_t *drp = (volatile uint8_t *) &c->regs->DR;

  if (c->debug) {
    LOG(LL_DEBUG, ("len %d", (int) len));
  }

  /* Clear MODF error, if any, by reading SR. */;
  (void) c->regs->SR;
  /* Enable SPI in master mode with software SS control
   * (at this point CSx is already asserted). */
  SET_BIT(c->regs->CR1, SPI_CR1_SPE);

  while (len > 0) {
    uint8_t byte = *tx_data++;
    stm32_gspi_wait_tx_empty(c);
    if (c->debug) {
      LOG(LL_DEBUG, ("write 0x%02x", byte));
    }
    *drp = byte;
    while (!(c->regs->SR & SPI_SR_RXNE)) {
    }
    while ((c->regs->SR & SPI_SR_RXNE)) {
      byte = *drp;
      if (c->debug) {
        LOG(LL_DEBUG, ("read 0x%02x", byte));
      }
      *rx_data++ = byte;
    }
    len--;
  }

  return true;
}

bool stm32_gspi_run_txn_hd(struct mgos_spi *c, const struct mgos_spi_txn *txn) {
  const uint8_t *tx_data = (const uint8_t *) txn->hd.tx_data;
  size_t tx_len = txn->hd.tx_len;
  size_t dummy_len = txn->hd.dummy_len;
  uint8_t *rx_data = (uint8_t *) txn->hd.rx_data;
  size_t rx_len = txn->hd.rx_len;
  volatile uint8_t *drp = (volatile uint8_t *) &c->regs->DR;
  if (c->debug) {
    LOG(LL_DEBUG, ("tx_len %d dummy_len %d rx_len %d", (int) tx_len,
                   (int) dummy_len, (int) rx_len));
  }

  /* Clear MODF error, if any, by reading SR. */;
  (void) c->regs->SR;
  /* Enable SPI in master mode with software SS control
   * (at this point CSx is already asserted). */
  SET_BIT(c->regs->CR1, SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_SPE);

  uint8_t byte;
  while (tx_len > 0) {
    byte = *tx_data++;
    stm32_gspi_wait_tx_empty(c);
    if (c->debug) {
      LOG(LL_DEBUG, ("write 0x%02x", byte));
    }
    *drp = byte;
    tx_len--;
  }

  while (dummy_len > 0) {
    /* Clock is only output when there's data to transmit so we send out 0s. */
    stm32_gspi_wait_tx_empty(c);
    *drp = 0;
    dummy_len--;
  }

  /* Wait for tx_data and dummy bytes to finish transmitting. */
  stm32_gspi_wait_tx_empty(c);
  while (c->regs->SR & SPI_SR_BSY) {
  }
  /* Empty the RX FIFO */
  while ((c->regs->SR & SPI_SR_RXNE) != 0) {
    byte = (uint8_t) c->regs->DR;
  }

  if (rx_len > 0) {
    do {
      stm32_gspi_wait_tx_empty(c);
      /* Dummy data to provide clock. */
      *drp = 0;
      while (!(c->regs->SR & SPI_SR_RXNE)) {
      }
      byte = *drp;
      if (c->debug) {
        LOG(LL_DEBUG, ("read 0x%02x", byte));
      }
      *rx_data++ = byte;
      if ((c->regs->SR & SPI_SR_RXNE)) {
        /* We expect exactly one byte RX for one byte TX.
         * Something went out of sync. */
        return false;
      }
      rx_len--;
    } while (rx_len > 0);
  }

  return true;
}
