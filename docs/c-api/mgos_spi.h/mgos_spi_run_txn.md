---
title: "mgos_spi_run_txn()"
decl_name: "mgos_spi_run_txn"
symbol_kind: "func"
signature: |
  bool mgos_spi_run_txn(struct mgos_spi *spi, bool full_duplex,
                        const struct mgos_spi_txn *txn);
---

Execute a half-duplex transaction. 

