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

#include "stm32_spi_master_internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/cs_dbg.h"

#include "mgos_gpio.h"
#include "mgos_system.h"
#include "mgos_sys_config.h"

struct mgos_spi *mgos_spi_create(const struct mgos_config_spi *cfg) {
  struct mgos_spi *c = (struct mgos_spi *) calloc(1, sizeof(*c));
  if (c == NULL) goto out_err;
  volatile uint32_t *apb_en_reg, *apb_rst_reg;
  uint32_t apb_en_bit, apb_rst_bit;

  switch (cfg->unit_no) {
#ifdef SPI1
    case 1:
      c->regs = SPI1;
#if defined(RCC_APB1ENR_SPI1EN)
      apb_en_reg = &RCC->APB1ENR;
      apb_en_bit = RCC_APB1ENR_SPI1EN;
      apb_rst_reg = &RCC->APB1RSTR;
      apb_rst_bit = RCC_APB1RSTR_SPI1RST;
#elif defined(RCC_APB2ENR_SPI1EN)
      apb_en_reg = &RCC->APB2ENR;
      apb_en_bit = RCC_APB2ENR_SPI1EN;
      apb_rst_reg = &RCC->APB2RSTR;
      apb_rst_bit = RCC_APB2RSTR_SPI1RST;
#else
#error Unknown SPI1 APB config
#endif
      break;
#endif /* SPI1 */
#ifdef SPI2
    case 2:
      c->regs = SPI2;
#if defined(RCC_APB1ENR_SPI2EN)
      apb_en_reg = &RCC->APB1ENR;
      apb_en_bit = RCC_APB1ENR_SPI2EN;
      apb_rst_reg = &RCC->APB1RSTR;
      apb_rst_bit = RCC_APB1RSTR_SPI2RST;
#elif defined(RCC_APB1ENR1_SPI2EN)
      apb_en_reg = &RCC->APB1ENR1;
      apb_en_bit = RCC_APB1ENR1_SPI2EN;
      apb_rst_reg = &RCC->APB1RSTR1;
      apb_rst_bit = RCC_APB1RSTR1_SPI2RST;
#elif defined(RCC_APB2ENR_SPI2EN)
      apb_en_reg = &RCC->APB2ENR;
      apb_en_bit = RCC_APB2ENR_SPI2EN;
      apb_rst_reg = &RCC->APB2RSTR;
      apb_rst_bit = RCC_APB2RSTR_SPI2RST;
#else
#error Unknown SPI2 APB config
#endif
      break;
#endif /* SPI2 */
#ifdef SPI3
    case 3:
      c->regs = SPI3;
#if defined(RCC_APB1ENR_SPI3EN)
      apb_en_reg = &RCC->APB1ENR;
      apb_en_bit = RCC_APB1ENR_SPI3EN;
      apb_rst_reg = &RCC->APB1RSTR;
      apb_rst_bit = RCC_APB1RSTR_SPI3RST;
#elif defined(RCC_APB1ENR1_SPI3EN)
      apb_en_reg = &RCC->APB1ENR1;
      apb_en_bit = RCC_APB1ENR1_SPI3EN;
      apb_rst_reg = &RCC->APB1RSTR1;
      apb_rst_bit = RCC_APB1RSTR1_SPI3RST;
#elif defined(RCC_APB2ENR_SPI3EN)
      apb_en_reg = &RCC->APB2ENR;
      apb_en_bit = RCC_APB2ENR_SPI3EN;
      apb_rst_reg = &RCC->APB2RSTR;
      apb_rst_bit = RCC_APB2RSTR_SPI3RST;
#else
#error Unknown SPI3 APB config
#endif
      break;
#endif /* SPI3 */
#ifdef SPI4
    case 4:
      c->regs = SPI4;
#if defined(RCC_APB1ENR_SPI4EN)
      apb_en_reg = &RCC->APB1ENR;
      apb_en_bit = RCC_APB1ENR_SPI4EN;
      apb_rst_reg = &RCC->APB1RSTR;
      apb_rst_bit = RCC_APB1RSTR_SPI4RST;
#elif defined(RCC_APB2ENR_SPI4EN)
      apb_en_reg = &RCC->APB2ENR;
      apb_en_bit = RCC_APB2ENR_SPI4EN;
      apb_rst_reg = &RCC->APB2RSTR;
      apb_rst_bit = RCC_APB2RSTR_SPI4RST;
#else
#error Unknown SPI4 APB config
#endif
      break;
#endif /* SPI4 */
#ifdef SPI5
    case 5:
      c->regs = SPI5;
#if defined(RCC_APB1ENR_SPI5EN)
      apb_en_reg = &RCC->APB1ENR;
      apb_en_bit = RCC_APB1ENR_SPI5EN;
      apb_rst_reg = &RCC->APB1RSTR;
      apb_rst_bit = RCC_APB1RSTR_SPI5RST;
#elif defined(RCC_APB2ENR_SPI5EN)
      apb_en_reg = &RCC->APB2ENR;
      apb_en_bit = RCC_APB2ENR_SPI5EN;
      apb_rst_reg = &RCC->APB2RSTR;
      apb_rst_bit = RCC_APB2RSTR_SPI5RST;
#else
#error Unknown SPI5 APB config
#endif
      break;
#endif /* SPI5 */
#ifdef SPI6
    case 6:
      c->regs = SPI6;
#if defined(RCC_APB1ENR_SPI6EN)
      apb_en_reg = &RCC->APB1ENR;
      apb_en_bit = RCC_APB1ENR_SPI6EN;
      apb_rst_reg = &RCC->APB1RSTR;
      apb_rst_bit = RCC_APB1RSTR_SPI6RST;
#elif defined(RCC_APB2ENR_SPI6EN)
      apb_en_reg = &RCC->APB2ENR;
      apb_en_bit = RCC_APB2ENR_SPI6EN;
      apb_rst_reg = &RCC->APB2RSTR;
      apb_rst_bit = RCC_APB2RSTR_SPI6RST;
#else
#error Unknown SPI6 APB config
#endif
      break;
#endif /* SPI6 */
#ifdef QUADSPI
    case STM32_QSPI_UNIT_NO:
      c->qregs = QUADSPI;
      apb_en_reg = &RCC->AHB3ENR;
      apb_en_bit = RCC_AHB3ENR_QSPIEN;
      apb_rst_reg = &RCC->AHB3RSTR;
      apb_rst_bit = RCC_AHB3RSTR_QSPIRST;
      break;
#endif /* QUADSPI */
    default:
      LOG(LL_ERROR, ("Invalid unit_no %d", cfg->unit_no));
      goto out_err;
  }
  c->unit_no = cfg->unit_no;

  if (cfg->sclk_gpio < 0 || (cfg->miso_gpio < 0 && cfg->mosi_gpio < 0)) {
    LOG(LL_ERROR, ("Invalid pin settings %d %d %d", cfg->sclk_gpio,
                   cfg->miso_gpio, cfg->mosi_gpio));
    goto out_err;
  }

  *apb_en_reg |= apb_en_bit;
  *apb_rst_reg |= apb_rst_bit;
  c->apb_en_reg = apb_en_reg;
  c->apb_en_bit = apb_en_bit;
  *apb_rst_reg &= ~apb_rst_bit;

  if (cfg->miso_gpio >= 0) {
    mgos_gpio_set_mode(cfg->miso_gpio, MGOS_GPIO_MODE_INPUT);
    mgos_gpio_set_pull(cfg->miso_gpio, MGOS_GPIO_PULL_UP);
  }
  if (cfg->mosi_gpio >= 0) {
    mgos_gpio_set_mode(cfg->mosi_gpio, MGOS_GPIO_MODE_OUTPUT);
  }
  if (cfg->sclk_gpio >= 0) {
    mgos_gpio_set_mode(cfg->sclk_gpio, MGOS_GPIO_MODE_OUTPUT);
    c->sclk_gpio = cfg->sclk_gpio;
  }
  c->cs_gpio[0] = cfg->cs0_gpio;
  if (cfg->cs0_gpio >= 0) {
    mgos_gpio_set_mode(cfg->cs0_gpio, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_write(cfg->cs0_gpio, 1);
  }
  c->cs_gpio[1] = cfg->cs1_gpio;
  if (cfg->cs1_gpio >= 0) {
    mgos_gpio_set_mode(cfg->cs1_gpio, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_write(cfg->cs1_gpio, 1);
  }
  c->cs_gpio[2] = cfg->cs2_gpio;
  if (cfg->cs2_gpio >= 0) {
    mgos_gpio_set_mode(cfg->cs2_gpio, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_write(cfg->cs2_gpio, 1);
  }
#ifdef QUADSPI
  if (c->unit_no == STM32_QSPI_UNIT_NO) {
    if (cfg->qspi_io2 >= 0) {
      mgos_gpio_set_mode(cfg->qspi_io2, MGOS_GPIO_MODE_OUTPUT);
    }
    if (cfg->qspi_io3 >= 0) {
      mgos_gpio_set_mode(cfg->qspi_io3, MGOS_GPIO_MODE_OUTPUT);
    }
  }
#endif

  if (!mgos_spi_configure(c, cfg)) {
    goto out_err;
  }

  char b1[8], b2[8], b3[8], b4[8], b5[8], b6[8];
  LOG(LL_INFO,
      ("%sSPI%d init ok (MISO: %s, MOSI: %s, SCLK: %s; "
       "CS0/1/2: %s/%s/%s)",
       (cfg->unit_no == STM32_QSPI_UNIT_NO ? "Q" : ""), (cfg->unit_no & 0x7f),
       mgos_gpio_str(cfg->miso_gpio, b1), mgos_gpio_str(cfg->mosi_gpio, b2),
       mgos_gpio_str(cfg->sclk_gpio, b3), mgos_gpio_str(cfg->cs0_gpio, b4),
       mgos_gpio_str(cfg->cs1_gpio, b5), mgos_gpio_str(cfg->cs2_gpio, b6)));
  (void) b1;
  (void) b2;
  (void) b3;
  (void) b4;
  (void) b5;
  (void) b6;
  return c;

out_err:
  free(c);
  LOG(LL_ERROR, ("Invalid SPI settings"));
  return NULL;
}

bool mgos_spi_configure(struct mgos_spi *c, const struct mgos_config_spi *cf) {
  return IS_QSPI(c) ? stm32_qspi_configure(c, cf) : stm32_gspi_configure(c, cf);
}

static bool stm32_spi_set_freq(struct mgos_spi *c, int f) {
  if (c->freq == f) return true;
  return IS_QSPI(c) ? stm32_qspi_set_freq(c, f) : stm32_gspi_set_freq(c, f);
}

static bool stm32_spi_set_mode(struct mgos_spi *c, int m) {
  return IS_QSPI(c) ? stm32_qspi_set_mode(c, m) : stm32_gspi_set_mode(c, m);
}

bool mgos_spi_run_txn(struct mgos_spi *c, bool full_duplex,
                      const struct mgos_spi_txn *txn) {
  bool ret = false;
  int cs_gpio = -1;
  if (txn->cs >= 0) {
    if (txn->cs > 2) return false;
    cs_gpio = c->cs_gpio[txn->cs];
    if (cs_gpio < 0) return false;
  }
  if (txn->freq > 0 && !stm32_spi_set_freq(c, txn->freq)) {
    return false;
  }
  if (!stm32_spi_set_mode(c, txn->mode)) {
    return false;
  }
  if (cs_gpio > 0) {
    mgos_gpio_write(cs_gpio, 0);
  }
  if (full_duplex) {
    ret = (IS_QSPI(c) ? stm32_qspi_run_txn_fd(c, txn)
                      : stm32_gspi_run_txn_fd(c, txn));
  } else {
    ret = (IS_QSPI(c) ? stm32_qspi_run_txn_hd(c, txn)
                      : stm32_gspi_run_txn_hd(c, txn));
  }
  if (cs_gpio > 0) {
    mgos_gpio_write(cs_gpio, 1);
  }
  if (IS_QSPI(c)) {
#ifdef QUADSPI
    CLEAR_BIT(c->qregs->CR, QUADSPI_CR_EN);
#endif
  } else {
    CLEAR_BIT(c->regs->CR1, SPI_CR1_SPE);
  }
  return ret;
}

void mgos_spi_close(struct mgos_spi *c) {
  if (c == NULL) return;
  *c->apb_en_reg &= ~c->apb_en_bit;
  free(c);
}
