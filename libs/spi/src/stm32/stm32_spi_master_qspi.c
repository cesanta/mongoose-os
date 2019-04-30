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

#ifdef QUADSPI

bool stm32_qspi_configure(struct mgos_spi *c,
                          const struct mgos_config_spi *cfg) {
  c->qregs->CR = 0;
  c->qregs->DCR = 0;
  /* We do not use address phase in our transactions but still need to set
   * chip size or controller raises invalid address error. */
  c->qregs->DCR = (31 << QUADSPI_DCR_FSIZE_Pos);
  c->debug = cfg->debug;
  return true;
}

bool stm32_qspi_set_freq(struct mgos_spi *c, int freq) {
  CLEAR_BIT(c->qregs->CR, QUADSPI_CR_EN);
  int eff_freq = HAL_RCC_GetHCLKFreq();
  int div = eff_freq / freq;
  if (eff_freq / div > freq) div++;
  if (div > 256) return false;
  eff_freq = eff_freq / div;
  uint32_t br = (uint32_t) div - 1;
  MODIFY_REG(c->qregs->CR, QUADSPI_CR_PRESCALER_Msk,
             br << QUADSPI_CR_PRESCALER_Pos);
  c->freq = freq;
  if (c->debug) {
    LOG(LL_DEBUG, ("freq %d => div %d (br %d) => eff_freq %d", c->freq, div,
                   (int) br, eff_freq));
  }
  return true;
}

bool stm32_qspi_set_mode(struct mgos_spi *c, int mode) {
  if (mode == 0) {
    CLEAR_BIT(c->qregs->DCR, QUADSPI_DCR_CKMODE);
  } else if (mode == 3) {
    SET_BIT(c->qregs->DCR, QUADSPI_DCR_CKMODE);
  } else {
    return false;
  }
  return true;
}

bool stm32_qspi_run_txn_fd(struct mgos_spi *c, const struct mgos_spi_txn *txn) {
  /* Not supported */
  (void) c;
  (void) txn;
  return false;
}

static inline uint32_t stm32_qspi_fifo_len(const struct mgos_spi *c) {
/* NB: QUADSPI_SR_FLEVEL_Msk is defined as 0x1F << QUADSPI_SR_FLEVEL_Pos
 * which is incorrect. The field is actually 6 bits wide! */
#define QUADSPI_SR_FLEVEL_Msk_2 (0x3FU << QUADSPI_SR_FLEVEL_Pos)
  return ((c->qregs->SR & QUADSPI_SR_FLEVEL_Msk_2) >> QUADSPI_SR_FLEVEL_Pos);
}

bool stm32_qspi_run_txn_hd(struct mgos_spi *c, const struct mgos_spi_txn *txn) {
  const uint8_t *tx_data = (const uint8_t *) txn->hd.tx_data;
  size_t tx_len = txn->hd.tx_len;
  size_t dummy_len = txn->hd.dummy_len;
  uint8_t *rx_data = (uint8_t *) txn->hd.rx_data;
  size_t rx_len = txn->hd.rx_len;
  volatile uint8_t *drp = (volatile uint8_t *) &c->qregs->DR;
  if (c->debug) {
    LOG(LL_DEBUG, ("tx_len %d dummy_len %d rx_len %d", (int) tx_len,
                   (int) dummy_len, (int) rx_len));
  }
  SET_BIT(c->qregs->CR, QUADSPI_CR_EN);
  if (tx_len > 0) {
    c->qregs->FCR = QUADSPI_FCR_CTCF;
    /* Indirect write fmode (0), data phase only, single line dmode (1). */
    c->qregs->DLR = tx_len + dummy_len - 1;
    c->qregs->CCR = QSPI_DATA_1_LINE;
    while (tx_len > 0) {
      /* To avoid blocking the CPU we avoid filling up the FIFO */
      while (stm32_qspi_fifo_len(c) > 30) {
      }
      *drp = *tx_data++;
      tx_len--;
    }
    while (dummy_len > 0) {
      while (stm32_qspi_fifo_len(c) > 30) {
      }
      *drp = 0;
      dummy_len--;
    }
    while (!(c->qregs->SR & QUADSPI_SR_TCF)) {
    }
  }
  if (rx_len > 0) {
    c->qregs->FCR = QUADSPI_FCR_CTCF;
    /* Indirect read fmode (1), data phase only, single line dmode (1). */
    c->qregs->DLR = rx_len - 1;
    c->qregs->CCR = ((1 << QUADSPI_CCR_FMODE_Pos) | QSPI_DATA_1_LINE);
    while (rx_len > 0) {
      /* To avoid blocking the CPU we wait for data to become available. */
      while (stm32_qspi_fifo_len(c) == 0) {
      }
      *rx_data++ = *drp;
      rx_len--;
    }
  }
  return true;
}

#endif /* QUADSPI */
