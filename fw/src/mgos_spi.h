/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_SPI_H_
#define CS_FW_SRC_MGOS_SPI_H_

#include "fw/src/mgos_features.h"

#if MGOS_ENABLE_SPI

#include <stdint.h>

#include "fw/src/mgos_init.h"
#include "fw/src/mgos_sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_spi;

/* Initialize SPI master */
struct mgos_spi *mgos_spi_create(const struct sys_config_spi *cfg);

/* (Re)configure existing SPI interface. */
bool mgos_spi_configure(struct mgos_spi *spi, const struct sys_config_spi *cfg);

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

/* Execute a half-duplex transaction. */
bool mgos_spi_run_txn(struct mgos_spi *spi, bool full_duplex,
                      const struct mgos_spi_txn *txn);

void mgos_spi_close(struct mgos_spi *spi);

enum mgos_init_result mgos_spi_init(void);

struct mgos_spi *mgos_spi_get_global(void);

#ifdef __cplusplus
}
#endif

#endif /* MGOS_ENABLE_SPI */

#endif /* CS_FW_SRC_MGOS_SPI_H_ */
