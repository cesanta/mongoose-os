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
#include <stdint.h>

#include "common/mg_str.h"

#include "mgos_sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_spi;

/* Initialize SPI master. */
struct mgos_spi *mgos_spi_create(const struct mgos_config_spi *cfg);

/* (Re)configure existing SPI interface. */
bool mgos_spi_configure(struct mgos_spi *spi,
                        const struct mgos_config_spi *cfg);

struct mgos_spi_txn {
  /* Which CS line to use, 0, 1 or 2. use -1 to not assert any CS
   * during transaction, it is assumed to be done externally.
   * Note: this is not a GPIO number, mapping from cs to GPIO is set by
   * mgos_spi_configure. */
  int cs;
  /* Mode, 0-3. This controls clock phase and polarity. */
  int mode;
  /* Clock frequency to use. 0 means don't change. */
  int freq;
  union {
    /*
     * Half-duplex SPI transaction proceeds in phases: transmit, wait and
     * receive.
     *  - First, tx_len bytes from *tx_data are transmitted
     *  - Optionally, a number of dummy cycles are inserted (in multiples of 8)
     *  - Then rx_len bytes are received into *rx_data.
     * Either tx_len or rx_len can be 0, in which case corresponding pointer can
     * be
     * NULL too.
     */
    struct {
      /* Data to transmit, number of bytes and buffer pointer. Can be 0 and
       * NULL. */
      size_t tx_len;
      const void *tx_data;
      /* Insert the specified number of dummy bytes (multiples of 8 cycles). */
      size_t dummy_len;
      /* Number of bytes and destination buffer for the data. Can be 0 and NULL.
       */
      size_t rx_len;
      void *rx_data;
    } hd;
    /*
     * In a full-duplex SPI transaction data is sent and received at the same
     * time:
     * for every byte sent from *tx_data a byte is stored into *rx_data.
     * Passing the same pointer for tx_data nd rx_data is ok.
     * rx_data may also be NULL, in which case this effectively becomes a simple
     * tx-only transaction.
     */
    struct {
      size_t len;
      const void *tx_data;
      void *rx_data;
    } fd;
  };
};

/*
 * Execute a half-duplex transaction. See `struct mgos_spi_txn` for the details
 * on transaction params.
 */
bool mgos_spi_run_txn(struct mgos_spi *spi, bool full_duplex,
                      const struct mgos_spi_txn *txn);

/* Close SPI handle. */
void mgos_spi_close(struct mgos_spi *spi);

/* Return global SPI bus handle which is configured via sysconfig. */
struct mgos_spi *mgos_spi_get_global(void);

bool mgos_spi_config_from_json(const struct mg_str cfg_json,
                               struct mgos_config_spi *cfg);

#ifdef __cplusplus
}
#endif
