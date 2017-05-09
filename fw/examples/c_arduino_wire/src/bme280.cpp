/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 * This Arduino like Wire library example shows how can use
 * the BOSCH BME280 combined humidity and pressure sensor.
 * Datasheet: https://ae-bst.resource.bosch.com/media/_tech/media/
 *            datasheets/BST-BME280_DS001-11.pdf
 */

#include "bme280.h"

// Compensation parameters
uint16_t dig_t1;
int16_t dig_t2, dig_t3;

uint16_t dig_p1;
int16_t dig_p2, dig_p3, dig_p4, dig_p5, dig_p6, dig_p7, dig_p8, dig_p9;

uint8_t dig_h1;
int16_t dig_h2;
uint8_t dig_h3;
int16_t dig_h4;
int16_t dig_h5;
int8_t dig_h6;

// t_fine stores a fine temperature for calculations
int64_t t_fine;

bool init(void) {
  // Check the chip identification number
  if (read_byte(ADDR, REG_ID) != BME280_CHIP_ID) return false;

  // Reset the device
  write_byte(ADDR, REG_RESET, CMD_RESET);

  delay(300);

  // We should wait until the sensor finished copied NVM
  // (non-volatile memory) data to image registers.
  while ((read_byte(ADDR, REG_STATUS) & (1 << 0)) != 0) delay(100);

  // Read compensation parameters
  dig_t1 = read_word_le(ADDR, REG_DIG_T1);
  dig_t2 = read_word_le(ADDR, REG_DIG_T2);
  dig_t3 = read_word_le(ADDR, REG_DIG_T3);

  dig_p1 = read_word_le(ADDR, REG_DIG_P1);
  dig_p2 = read_word_le(ADDR, REG_DIG_P2);
  dig_p3 = read_word_le(ADDR, REG_DIG_P3);
  dig_p4 = read_word_le(ADDR, REG_DIG_P4);
  dig_p5 = read_word_le(ADDR, REG_DIG_P5);
  dig_p6 = read_word_le(ADDR, REG_DIG_P6);
  dig_p7 = read_word_le(ADDR, REG_DIG_P7);
  dig_p8 = read_word_le(ADDR, REG_DIG_P8);
  dig_p9 = read_word_le(ADDR, REG_DIG_P9);

  dig_h1 = read_byte(ADDR, REG_DIG_H1);
  dig_h2 = read_word_le(ADDR, REG_DIG_H2);
  dig_h3 = read_byte(ADDR, REG_DIG_H3);
  dig_h4 = (read_byte(ADDR, REG_DIG_H4) << 4) |
           (read_byte(ADDR, REG_DIG_H4 + 1) & 0xF);
  dig_h5 = (read_byte(ADDR, REG_DIG_H5 + 1) << 4) |
           (read_byte(ADDR, REG_DIG_H5) >> 4);
  dig_h6 = (int8_t) read_byte(ADDR, REG_DIG_H6);

  // Setup a sensor with default parameters
  // (see the BME280 datasheet for more details)
  write_byte(ADDR, REG_CTRL_HUM, 0b00000101);
  write_byte(ADDR, REG_CONFIG, 0b00000000);
  write_byte(ADDR, REG_CTRL_MEAS, 0b10110111);

  delay(100);

  return true;
}

// This function returns temperature in DegC, resolution is 0.01 DegC.
// t_fine carries fine temperature as global value
// https://ae-bst.resource.bosch.com/media/_tech/media/
// datasheets/BST-BME280_DS001-11.pdf P.23
float getTemp(void) {
  int32_t vat1, var2;
  int32_t adc_t = read(ADDR, REG_TEMP_MSB);
  if (adc_t == 0x800000) return -127;
  adc_t >>= 4;

  vat1 =
      ((((adc_t >> 3) - ((int32_t) dig_t1 << 1))) * ((int32_t) dig_t2)) >> 11;
  var2 = (((((adc_t >> 4) - ((int32_t) dig_t1)) *
            ((adc_t >> 4) - ((int32_t) dig_t1))) >>
           12) *
          ((int32_t) dig_t3)) >>
         14;

  t_fine = vat1 + var2;

  return (float) ((t_fine * 5 + 128) >> 8) / 100.0;
}

// This function returns humidity in %RH.
// https://ae-bst.resource.bosch.com/media/_tech/media/
// datasheets/BST-BME280_DS001-11.pdf P.23-24
float getHumi(void) {
  int32_t adc_h = read_word(ADDR, REG_HUM_MSB);
  if (adc_h == 0x8000) return 0;

  int32_t v_x1_u32r;

  v_x1_u32r = (t_fine - ((int32_t) 76800));
  v_x1_u32r =
      (((((adc_h << 14) - (((int32_t) dig_h4) << 20) -
          (((int32_t) dig_h5) * v_x1_u32r)) +
         ((int32_t) 16384)) >>
        15) *
       (((((((v_x1_u32r * ((int32_t) dig_h6)) >> 10) *
            (((v_x1_u32r * ((int32_t) dig_h3)) >> 11) + ((int32_t) 32768))) >>
           10) +
          ((int32_t) 2097152)) *
             ((int32_t) dig_h2) +
         8192) >>
        14));
  v_x1_u32r =
      (v_x1_u32r -
       (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t) dig_h1)) >>
        4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

  return (float) (v_x1_u32r >> 12) / 1024.0;
}

// This function returns pressure in kPa.
// https://ae-bst.resource.bosch.com/media/_tech/media/
// datasheets/BST-BME280_DS001-11.pdf P.23
float getPress(void) {
  int32_t adc_p = read(ADDR, REG_PRESS_MSB);
  if (adc_p == 0x800000) return 0;
  adc_p >>= 4;

  int64_t var1, var2, p;
  var1 = ((int64_t) t_fine) - 128000;
  var2 = var1 * var1 * (int64_t) dig_p6;
  var2 = var2 + ((var1 * (int64_t) dig_p5) << 17);
  var2 = var2 + (((int64_t) dig_p4) << 35);
  var1 = ((var1 * var1 * (int64_t) dig_p3) >> 8) +
         ((var1 * (int64_t) dig_p2) << 12);
  var1 = (((((int64_t) 1) << 47) + var1)) * ((int64_t) dig_p1) >> 33;
  if (var1 == 0) return 0;  // avoid exception caused by division by zero
  p = 1048576 - adc_p;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t) dig_p9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t) dig_p8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t) dig_p7) << 4);
  return (float) p / 256000.0;
}
