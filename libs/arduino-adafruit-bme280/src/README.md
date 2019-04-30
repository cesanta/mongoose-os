Cesanta note

Imported from https://github.com/adafruit/Adafruit_BME280_Library/commit/a148538212b07df05881562874171de7f312c81f

This is a library for the Adafruit BME280 Humidity, Barometric Pressure + Temp sensor

Designed specifically to work with the Adafruit BME280 Breakout 
 * http://www.adafruit.com/products/2652

These sensors use I2C or SPI to communicate, up to 4 pins are required to interface

Use of this library also requires [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
to be installed on your local system.

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Check out the links above for our tutorials and wiring diagrams 

Written by Limor Fried/Ladyada for Adafruit Industries.  
BSD license, all text above must be included in any redistribution

To download. click the DOWNLOAD ZIP button, rename the uncompressed folder Adafruit_BME280. 
Check that the Adafruit_BME280 folder contains Adafruit_BME280.cpp and Adafruit_BME280.h

Place the Adafruit_BME280 library folder your arduinosketchfolder/libraries/ folder. 
You may need to create the libraries subfolder if its your first library. Restart the IDE.

We also have a great tutorial on Arduino library installation at:
http://learn.adafruit.com/adafruit-all-about-arduino-libraries-install-use
<!-- START COMPATIBILITY TABLE -->

## Compatibility

MCU                | Tested Works | Doesn't Work | Not Tested  | Notes
------------------ | :----------: | :----------: | :---------: | -----
Atmega328 @ 16MHz  |      X       |             |            | 
Atmega328 @ 12MHz  |      X       |             |            | 
Atmega32u4 @ 16MHz |      X       |             |            | Use SDA/SCL on pins D2 &amp; D3
Atmega32u4 @ 8MHz  |      X       |             |            | Use SDA/SCL on pins D2 &amp; D3
ESP8266            |      X       |             |            | I2C: just works, SPI: SDA/SCL default to pins 4 &amp; 5 but any two pins can be assigned as SDA/SCL using Wire.begin(SDA,SCL)
ESP32              |      X       |             |            | I2C: just works, SPI: SDA/SCL default to pins 4 &amp; 5 but any two pins can be assigned as SDA/SCL using Wire.begin(SDA,SCL) 
Atmega2560 @ 16MHz |      X       |             |            | Use SDA/SCL on pins 20 &amp; 21
ATSAM3X8E          |      X       |             |            | Use SDA/SCL on pins 20 &amp; 21
ATSAM21D           |      X       |             |            | 
ATtiny85 @ 16MHz   |              |     X       |            | 
ATtiny85 @ 8MHz    |              |     X       |            | 
Intel Curie @ 32MHz |             |             |     X      | 
STM32F2            |              |             |     X      | 

  * ATmega328 @ 16MHz : Arduino UNO, Adafruit Pro Trinket 5V, Adafruit Metro 328, Adafruit Metro Mini
  * ATmega328 @ 12MHz : Adafruit Pro Trinket 3V
  * ATmega32u4 @ 16MHz : Arduino Leonardo, Arduino Micro, Arduino Yun, Teensy 2.0
  * ATmega32u4 @ 8MHz : Adafruit Flora, Bluefruit Micro
  * ESP8266 : Adafruit Huzzah
  * ATmega2560 @ 16MHz : Arduino Mega
  * ATSAM3X8E : Arduino Due
  * ATSAM21D : Arduino Zero, M0 Pro
  * ATtiny85 @ 16MHz : Adafruit Trinket 5V
  * ATtiny85 @ 8MHz : Adafruit Gemma, Arduino Gemma, Adafruit Trinket 3V

<!-- END COMPATIBILITY TABLE -->
