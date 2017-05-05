---
title: "mgos_spi_txn()"
decl_name: "mgos_spi_txn"
symbol_kind: "func"
signature: |
  bool mgos_spi_txn(struct mgos_spi *spi, const void *tx_data, size_t tx_len,
                    void *rx_data, size_t rx_len);
---

SPI transaction proceeds in two phases: transmit and receive.
First, tx_len bytes from *tx_data are transmitted, then rx_len bytes are
*received into *rx_data.
Either tx_len or rx_len can be 0, in which case corresponding pointer can be
*NULL too.

Note that slave selection is out of scope of this function, so appropriate
*slave
should already be selected (CS driven low, usually via GPIO) prior to
*invoking mgos_spi_txn. 

