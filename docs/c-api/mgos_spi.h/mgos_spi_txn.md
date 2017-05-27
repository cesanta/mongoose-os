---
title: "mgos_spi_txn()"
decl_name: "mgos_spi_txn"
symbol_kind: "func"
signature: |
  bool mgos_spi_txn(struct mgos_spi *spi, const void *tx_data, void *rx_data,
                    size_t len);
---

In a full-duplex SPI transaction data is sent and received at the same time:
for every byte sent from *tx_data a byte is stored into *rx_data.
Passing the same pointer for tx_data nd rx_data is ok.
rx_data may also be NULL if received data does not need to be stored.

Note that slave selection is out of scope of this function, so appropriate
slave should already be selected (CS set to active, usually via GPIO). 

