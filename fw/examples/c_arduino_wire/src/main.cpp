/*
 * This Arduino like Wire library example shows how can use
 * the BOSCH BME280 combined humidity and pressure sensor.
 * Datasheet: https://ae-bst.resource.bosch.com/media/_tech/media/
 *            datasheets/BST-BME280_DS001-11.pdf
 * 
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <Wire.h>

enum {
  ADDR          = 0x76,

  REG_DIG_T1    = 0x88,
  REG_DIG_T2    = 0x8A,
  REG_DIG_T3    = 0x8C,

  REG_DIG_P1    = 0x8E,
  REG_DIG_P2    = 0x90,
  REG_DIG_P3    = 0x92,
  REG_DIG_P4    = 0x94,
  REG_DIG_P5    = 0x96,
  REG_DIG_P6    = 0x98,
  REG_DIG_P7    = 0x9A,
  REG_DIG_P8    = 0x9C,
  REG_DIG_P9    = 0x9E,

  REG_DIG_H1    = 0xA1,
  REG_DIG_H2    = 0xE1,
  REG_DIG_H3    = 0xE3,
  REG_DIG_H4    = 0xE4,
  REG_DIG_H5    = 0xE5,
  REG_DIG_H6    = 0xE7,

  REG_CHIPID    = 0xD0,
  REG_SOFTRESET = 0xE0,
  REG_CTRLHUM   = 0xF2,
  REG_STATUS    = 0XF3,
  REG_CTRL      = 0xF4,
  REG_CONF      = 0xF5,
  REG_PDATA     = 0xF7,
  REG_TDATA     = 0xFA,
  REG_HDATA     = 0xFD
};

// Wire handle
TwoWire *w = NULL;

// Compensation parameters
uint16_t dig_t1;
int16_t  dig_t2,
         dig_t3;

uint16_t dig_p1;
int16_t  dig_p2,
         dig_p3,
         dig_p4,
         dig_p5,
         dig_p6,
         dig_p7,
         dig_p8,
         dig_p9;

uint8_t  dig_h1;
int16_t  dig_h2;
uint8_t  dig_h3;
int16_t  dig_h4;
int16_t  dig_h5;
int8_t   dig_h6;

// t_fine stores a fine temperature for calculations
int64_t t_fine;

void error(const char *s) {
  printf("Error: %s\n", s);
  delay(1000);
}

uint8_t read_byte(uint8_t reg) {
  w->beginTransmission(ADDR);
  w->write(reg);
  w->endTransmission();
  w->requestFrom((uint8_t)ADDR, (uint8_t)1);
  return w->read();
}

uint16_t read_word(uint8_t reg) {
  w->beginTransmission(ADDR);
  w->write(reg);
  w->endTransmission();
  w->requestFrom((uint8_t)ADDR, (uint8_t)2);
  return (w->read() << 8) | w->read();
}

uint16_t read_word_lit_end(byte reg) {
  uint16_t data = read_word(reg);
  return (data >> 8) | (data << 8);
}

uint32_t read(uint8_t reg) {
  uint32_t data;

  w->beginTransmission(ADDR);
  w->write(reg);
  w->endTransmission();
  w->requestFrom((uint8_t)ADDR, (uint8_t)3);

  data = w->read();
  data <<= 8;
  data |= w->read();
  data <<= 8;
  data |= w->read();

  return data;
}

void write_byte(uint8_t reg, uint8_t value) {
  w->beginTransmission(ADDR);
  w->write(reg);
  w->write(value);
  w->endTransmission();
}

void setup(void) {
  printf("Arduino Wire library example\n");

  // Initialize Wire library
  w = new TwoWire();
  w->begin();

  // Check the chip identification number
  if (read_byte(REG_CHIPID) != 0x60) {
    error("Can't find a sensor\n");
    while (true) delay(1000);
  }

  // Reset the device
  write_byte(REG_SOFTRESET, 0xB6);

  delay(300);

  // We should wait until the sensor finished copied NVM 
  // (non-volatile memory) data to image registers.
  while ((read_byte(REG_STATUS) & (1 << 0)) != 0) delay(100);
  
  // Read compensation parameters
  dig_t1 = read_word_lit_end(REG_DIG_T1);
  dig_t2 = read_word_lit_end(REG_DIG_T2);
  dig_t3 = read_word_lit_end(REG_DIG_T3);

  dig_p1 = read_word_lit_end(REG_DIG_P1);
  dig_p2 = read_word_lit_end(REG_DIG_P2);
  dig_p3 = read_word_lit_end(REG_DIG_P3);
  dig_p4 = read_word_lit_end(REG_DIG_P4);
  dig_p5 = read_word_lit_end(REG_DIG_P5);
  dig_p6 = read_word_lit_end(REG_DIG_P6);
  dig_p7 = read_word_lit_end(REG_DIG_P7);
  dig_p8 = read_word_lit_end(REG_DIG_P8);
  dig_p9 = read_word_lit_end(REG_DIG_P9);

  dig_h1 = read_byte(REG_DIG_H1);
  dig_h2 = read_word_lit_end(REG_DIG_H2);
  dig_h3 = read_byte(REG_DIG_H3);
  dig_h4 = (read_byte(REG_DIG_H4) << 4) | (read_byte(REG_DIG_H4+1) & 0xF);
  dig_h5 = (read_byte(REG_DIG_H5 + 1) << 4) | (read_byte(REG_DIG_H5) >> 4);
  dig_h6 = (int8_t)read_byte(REG_DIG_H6);

  // Setup a sensor with default parameters
  // (see the BME280 datasheet for more details)
  write_byte(REG_CTRLHUM, 0b00000101);
  write_byte(REG_CONF, 0b00000000);
  write_byte(REG_CTRL, 0b10110111);

  delay(100);
}

// This function reads temperature from the BME280 sensor
// Returns temperature in DegC, resolution is 0.01 DegC.
// t_fine carries fine temperature as global value
// https://ae-bst.resource.bosch.com/media/_tech/media/
// datasheets/BST-BME280_DS001-11.pdf P.23
float getTemp(void) {
  int32_t vat1, var2;
  int32_t adc_t = read(REG_TDATA);
  if (adc_t == 0x800000) return -127;
  adc_t >>= 4;

  vat1 = ((((adc_t >> 3) - ((int32_t)dig_t1 << 1))) * ((int32_t)dig_t2)) >> 11;
  var2 = (((((adc_t >> 4) - ((int32_t)dig_t1)) * ((adc_t >> 4) - ((int32_t)dig_t1))) >> 12) *
          ((int32_t)dig_t3)) >> 14;

  t_fine = vat1 + var2;

  return (float)((t_fine * 5 + 128) >> 8) / 100.0;
}

// This function reads humidity from the BME280 sensor
// Returns humidity in %RH.
// https://ae-bst.resource.bosch.com/media/_tech/media/
// datasheets/BST-BME280_DS001-11.pdf P.23-24
float getHumi(void) {
  int32_t adc_h = read_word(REG_HDATA);
  if (adc_h == 0x8000) return 0;

  int32_t v_x1_u32r;

  v_x1_u32r = (t_fine - ((int32_t)76800));
  v_x1_u32r = (((((adc_h << 14) - (((int32_t)dig_h4) << 20) - (((int32_t)dig_h5) * v_x1_u32r)) +
              ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)dig_h6)) >> 10) * (((v_x1_u32r * 
              ((int32_t)dig_h3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) *
              ((int32_t)dig_h2) + 8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)dig_h1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

  return (float)(v_x1_u32r >> 12) / 1024.0;
}

// This function reads pressure from the BME280 sensor
// Returns pressure in kPa.
// https://ae-bst.resource.bosch.com/media/_tech/media/
// datasheets/BST-BME280_DS001-11.pdf P.23
float getPress(void) {
  int32_t adc_p = read(REG_PDATA);
  if (adc_p == 0x800000) return 0;
  adc_p >>= 4;

  int64_t var1, var2, p;
  var1 = ((int64_t)t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)dig_p6;
  var2 = var2 + ((var1 * (int64_t)dig_p5) << 17);
  var2 = var2 + (((int64_t)dig_p4) << 35);
  var1 = ((var1 * var1 * (int64_t)dig_p3) >> 8) + ((var1 * (int64_t)dig_p2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_p1) >> 33;
  if (var1 == 0) return 0; // avoid exception caused by division by zero
  p = 1048576 - adc_p;
  p = (((p<<31) - var2) * 3125) / var1;
  var1 = (((int64_t)dig_p9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)dig_p8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)dig_p7) << 4);
  return (float)p / 256000.0;
}

void loop(void) {
  printf("Temperature: %f *C\n", getTemp());
  printf("Humidity: %f %%RH\n", getHumi());
  printf("Pressure: %f kPa\n\n", getPress());
  delay(2000);
}
