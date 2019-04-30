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

#include "mgos_spi.h"

#include "stm32_sdk_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Special unit number that means QSPI */
#define STM32_QSPI_UNIT_NO 129

struct mgos_spi {
  int unit_no;
  int freq;
  int sclk_gpio;
  int cs_gpio[3];
  volatile SPI_TypeDef *regs;
#ifdef QUADSPI
  volatile QUADSPI_TypeDef *qregs;
#endif
  volatile uint32_t *apb_en_reg;
  uint32_t apb_en_bit;
  unsigned int mode : 2;
  unsigned int debug : 1;
};

#ifdef QUADSPI
#define IS_QSPI(c) ((c)->unit_no == STM32_QSPI_UNIT_NO)
#else
#define IS_QSPI(c) (false)
#endif

bool stm32_gspi_configure(struct mgos_spi *c,
                          const struct mgos_config_spi *cfg);
bool stm32_qspi_configure(struct mgos_spi *c,
                          const struct mgos_config_spi *cfg);

bool stm32_gspi_set_freq(struct mgos_spi *c, int freq);
bool stm32_qspi_set_freq(struct mgos_spi *c, int freq);

bool stm32_gspi_set_mode(struct mgos_spi *c, int mode);
bool stm32_qspi_set_mode(struct mgos_spi *c, int mode);

bool stm32_gspi_run_txn_fd(struct mgos_spi *c, const struct mgos_spi_txn *txn);
bool stm32_qspi_run_txn_fd(struct mgos_spi *c, const struct mgos_spi_txn *txn);

bool stm32_gspi_run_txn_hd(struct mgos_spi *c, const struct mgos_spi_txn *txn);
bool stm32_qspi_run_txn_hd(struct mgos_spi *c, const struct mgos_spi_txn *txn);

#ifdef __cplusplus
}
#endif
