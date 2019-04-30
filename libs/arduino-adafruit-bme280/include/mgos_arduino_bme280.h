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

#ifndef CS_MOS_LIBS_ARDUINO_ADAFRUIT_BME280_SRC_MGOS_ARDUINO_BME280_H_
#define CS_MOS_LIBS_ARDUINO_ADAFRUIT_BME280_SRC_MGOS_ARDUINO_BME280_H_

#ifdef __cplusplus
#include "Adafruit_BME280.h"
#else
typedef struct Adafruit_BME280Tag Adafruit_BME280;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_BME280_RES_FAIL -10000

/*
 * Initialize Adafruit_BME280 library for i2c.
 * Return value: OneWire handle opaque pointer.
 */
Adafruit_BME280 *mgos_bme280_create_i2c();

/*
 * Initialize Adafruit_BME280 library for spi with the given `cspin`.
 * Return value: OneWire handle opaque pointer.
 */
Adafruit_BME280 *mgos_bme280_create_spi(int cspin);

/*
 * Initialize Adafruit_BME280 library for spi with the given pins `cspin`,
 * `mosipin`, `misopin`, `sckpin`.
 * Return value: OneWire handle opaque pointer.
 */
Adafruit_BME280 *mgos_bme280_create_spi_full(int cspin, int mosipin,
                                             int misopin, int sckpin);

/*
 * Close Adafruit_BME280 handle. Return value: none.
 */
void mgos_bme280_close(Adafruit_BME280 *bme);

/*
 * Initialize the sensor with given parameters/settings.
 * Returns 0 if the sensor not plugged or a checking failed,
 * i.e. the chip ID is incorrect.
 * Returns 1 otherwise.
 */
int mgos_bme280_begin(Adafruit_BME280 *bme, int addr);

/*
 * Take a new measurement (only possible in forced mode).
 */
void mgos_bme280_take_forced_measurement(Adafruit_BME280 *bme);

/*
 * Returns the temperature from the sensor in degrees C * 100
 * or MGOS_BME280_RES_FAIL if an operation failed.
 */
int mgos_bme280_read_temperature(Adafruit_BME280 *bme);

/*
 * Returns the pressure from the sensor in hPa * 100
 * or MGOS_BME280_RES_FAIL if an operation failed.
 */
int mgos_bme280_read_pressure(Adafruit_BME280 *bme);

/*
 * Returns the humidity from the sensor in %RH * 100
 * or MGOS_BME280_RES_FAIL if an operation failed.
 */
int mgos_bme280_read_humidity(Adafruit_BME280 *bme);

/*
 * Returns the altitude in meters * 100 calculated from the specified
 * atmospheric pressure (in hPa), and sea-level pressure (in hPa * 100)
 * or MGOS_BME280_RES_FAIL if an operation failed.
 * http: *www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf, P.16
 */
int mgos_bme280_read_altitude(Adafruit_BME280 *bme, int seaLevel);

/*
 * Returns the pressure at sea level in hPa * 100
 * calculated from the specified altitude (in meters * 100),
 * and atmospheric pressure (in hPa * 100)
 * or MGOS_BME280_RES_FAIL if an operation failed.
 * http: *www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf, P.17
 */
int mgos_bme280_sea_level_for_altitude(Adafruit_BME280 *bme, int altitude,
                                       int pressure);

#ifdef __cplusplus
}
#endif

#endif /* CS_MOS_LIBS_ARDUINO_ADAFRUIT_BME280_SRC_MGOS_ARDUINO_BME280_H_ */
