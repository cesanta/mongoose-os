---
title: "mgos_i2c_read_reg_b()"
decl_name: "mgos_i2c_read_reg_b"
symbol_kind: "func"
signature: |
  int mgos_i2c_read_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg);
---

Register read/write routines.
These are helpers for reading register values from a device.
First a 1-byte write of a register address is performed, followed by either
a byte or a word (2 bytes) data read/write.
For word operations, value is big-endian: for a read, first byte read from
the bus is in bits 15-8, second is in bits 7-0; for a write, bits 15-8 are
put on the us first, followed by bits 7-0.
Read operations return negative number on error and positive value in the
respective range (0-255, 0-65535) on success. 

