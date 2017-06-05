/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * This Arduino like Wire library example shows how can use
 * the BOSCH BME280 combined humidity and pressure sensor.
 * Datasheet: https://ae-bst.resource.bosch.com/media/_tech/media/
 *            datasheets/BST-BME280_DS001-11.pdf
 */

#include <Wire.h>

// Wire handle
extern TwoWire *w;

extern uint8_t read_byte(uint8_t addr, uint8_t reg);
extern uint16_t read_word(uint8_t addr, uint8_t reg);
extern uint16_t read_word_le(uint8_t addr, uint8_t reg);
extern uint32_t read(uint8_t addr, uint8_t reg);
extern void write_byte(uint8_t addr, uint8_t reg, uint8_t value);

// Device address
enum bme280_address {
  ADDR = 0x76,
};

// Device memory map
// (https://ae-bst.resource.bosch.com/media/_tech/media/
// datasheets/BST-BME280_DS001-11.pdf, Table 18, P.26)
enum bme280_registers {
  REG_DIG_T1 = 0x88,
  REG_DIG_T2 = 0x8A,
  REG_DIG_T3 = 0x8C,

  REG_DIG_P1 = 0x8E,
  REG_DIG_P2 = 0x90,
  REG_DIG_P3 = 0x92,
  REG_DIG_P4 = 0x94,
  REG_DIG_P5 = 0x96,
  REG_DIG_P6 = 0x98,
  REG_DIG_P7 = 0x9A,
  REG_DIG_P8 = 0x9C,
  REG_DIG_P9 = 0x9E,

  REG_DIG_H1 = 0xA1,

  REG_ID = 0xD0,
  REG_RESET = 0xE0,

  REG_DIG_H2 = 0xE1,
  REG_DIG_H3 = 0xE3,
  REG_DIG_H4 = 0xE4,
  REG_DIG_H5 = 0xE5,
  REG_DIG_H6 = 0xE7,

  REG_CTRL_HUM = 0xF2,
  REG_STATUS = 0XF3,
  REG_CTRL_MEAS = 0xF4,
  REG_CONFIG = 0xF5,
  REG_PRESS_MSB = 0xF7,
  REG_TEMP_MSB = 0xFA,
  REG_HUM_MSB = 0xFD
};

// The BME280 chip identification number
enum bme280_chip_id { BME280_CHIP_ID = 0x60 };

// The complete power-on-reset procedure
enum bme280_cmd { CMD_RESET = 0xB6 };

bool init(void);
float getTemp(void);
float getHumi(void);
float getPress(void);
