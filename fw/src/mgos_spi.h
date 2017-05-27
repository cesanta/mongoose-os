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

/* (Re)configure exisint SPI interface. */
bool mgos_spi_configure(struct mgos_spi *spi, const struct sys_config_spi *cfg);
bool mgos_spi_set_freq(struct mgos_spi *spi, int freq);
bool mgos_spi_set_mode(struct mgos_spi *spi, int freq);
bool mgos_spi_set_msb_first(struct mgos_spi *spi, bool msb_first);

/*
 * In a full-duplex SPI transaction data is sent and received at the same time:
 * for every byte sent from *tx_data a byte is stored into *rx_data.
 * Passing the same pointer for tx_data nd rx_data is ok.
 * rx_data may also be NULL if received data does not need to be stored.
 *
 * Note that slave selection is out of scope of this function, so appropriate
 * slave should already be selected (CS set to active, usually via GPIO).
 */
bool mgos_spi_txn(struct mgos_spi *spi, const void *tx_data, void *rx_data,
                  size_t len);

/*
 * Half-duplex SPI transaction proceeds in two phases: transmit and receive.
 * First, tx_len bytes from *tx_data are transmitted, then rx_len bytes are
 * received into *rx_data.
 * Either tx_len or rx_len can be 0, in which case corresponding pointer can be
 * NULL too.
 *
 * Note that slave selection is out of scope of this function, so appropriate
 * slave should already be selected (CS set to active, usually via GPIO).
 */
bool mgos_spi_txn_hd(struct mgos_spi *spi, const void *tx_data, size_t tx_len,
                     void *rx_data, size_t rx_len);

void mgos_spi_close(struct mgos_spi *spi);

enum mgos_init_result mgos_spi_init(void);

struct mgos_spi *mgos_spi_get_global(void);

#ifdef __cplusplus
}
#endif

#endif /* MGOS_ENABLE_SPI */

#endif /* CS_FW_SRC_MGOS_SPI_H_ */
