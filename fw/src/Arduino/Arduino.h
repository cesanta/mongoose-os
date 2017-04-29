/*
 Arduino.h - Main include file for the Arduino SDK
 Copyright (c) 2005-2013 Arduino Team.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * Cesanta note: This file is copied from esp8266/Arduino
 */

/* clang-format off */
#ifndef Arduino_h
#define Arduino_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define HIGH 0x1
#define LOW 0x0

// GPIO FUNCTIONS
#define INPUT 0x00
#define INPUT_PULLUP 0x02
#define OUTPUT 0x01

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

// Interrupt Modes
#define RISING 0x01
#define FALLING 0x02
#define CHANGE 0x03
#define ONLOW 0x04
#define ONHIGH 0x05
#define ONLOW_WE 0x0C
#define ONHIGH_WE 0x0D

// undefine stdlib's abs if encountered
#ifdef abs
#undef abs
#endif

#define abs(x) ((x) > 0 ? (x) : -(x))
#define constrain(amt, low, high) \
  ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define round(x) ((x) >= 0 ? (long) ((x) + 0.5) : (long) ((x) -0.5))
#define radians(deg) ((deg) *DEG_TO_RAD)
#define degrees(rad) ((rad) *RAD_TO_DEG)
#define sq(x) ((x) * (x))

#ifndef __STRINGIFY
#define __STRINGIFY(a) #a
#endif

// these low level routines provide a replacement for SREG interrupt save that
// AVR uses
// but are esp8266 specific. A normal use pattern is like
//
//{
//    uint32_t savedPS = xt_rsil(1); // this routine will allow level 2 and
//    above
//    // do work here
//    xt_wsr_ps(savedPS); // restore the state
//}
//
// level (0-15), interrupts of the given level and above will be active
// level 15 will disable ALL interrupts,
// level 0 will enable ALL interrupts,
//
#define xt_rsil(level)                                                 \
  (__extension__({                                                     \
    uint32_t state;                                                    \
    __asm__ __volatile__("rsil %0," __STRINGIFY(level) : "=a"(state)); \
    state;                                                             \
  }))
#define xt_wsr_ps(state) \
  __asm__ __volatile__("wsr %0,ps; isync" ::"a"(state) : "memory")

#define interrupts() xt_rsil(0)
#define noInterrupts() xt_rsil(15)

#define clockCyclesPerMicrosecond() (F_CPU / 1000000L)
#define clockCyclesToMicroseconds(a) ((a) / clockCyclesPerMicrosecond())
#define microsecondsToClockCycles(a) ((a) *clockCyclesPerMicrosecond())

#define lowByte(w) ((uint8_t)((w) &0xff))
#define highByte(w) ((uint8_t)((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) \
  (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

// avr-libc defines _NOP() since 1.6.2
#ifndef _NOP
#define _NOP()               \
  do {                       \
    __asm__ volatile("nop"); \
  } while (0)
#endif

typedef unsigned int word;

#define bit(b) (1UL << (b))
#define _BV(b) (1UL << (b))

typedef uint8_t boolean;
typedef uint8_t byte;

// int atexit(void (*func)()) __attribute__((weak));

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
#ifdef TODO
int analogRead(uint8_t pin);
void analogReference(uint8_t mode);
void analogWrite(uint8_t pin, int val);
void analogWriteFreq(uint32_t freq);
void analogWriteRange(uint32_t range);
#endif

unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned int ms);
void delayMicroseconds(unsigned int us);
#ifdef TODO
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);

void attachInterrupt(uint8_t pin, void (*)(void), int mode);
void detachInterrupt(uint8_t pin);
#endif

void setup(void);
void loop(void);

#define digitalPinToPort(pin) (0)
#define digitalPinToBitMask(pin) (1UL << (pin))
#define digitalPinToTimer(pin) (0)
#define portOutputRegister(port) ((volatile uint32_t *) &GPO)
#define portInputRegister(port) ((volatile uint32_t *) &GPI)
#define portModeRegister(port) ((volatile uint32_t *) &GPE)

#define NOT_A_PIN -1
#define NOT_A_PORT -1
#define NOT_AN_INTERRUPT -1
#define NOT_ON_TIMER 0

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef TODO

#ifdef __cplusplus

#ifndef _GLIBCXX_VECTOR
// arduino is not compatible with std::vector
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define _min(a, b) ((a) < (b) ? (a) : (b))
#define _max(a, b) ((a) > (b) ? (a) : (b))

uint16_t makeWord(uint16_t w);
uint16_t makeWord(byte h, byte l);

#define word(...) makeWord(__VA_ARGS__)

unsigned long pulseIn(uint8_t pin, uint8_t state,
                      unsigned long timeout = 1000000L);
unsigned long pulseInLong(uint8_t pin, uint8_t state,
                          unsigned long timeout = 1000000L);

void tone(uint8_t _pin, unsigned int frequency, unsigned long duration = 0);
void noTone(uint8_t _pin);

// WMath prototypes
long random(long);
long random(long, long);
void randomSeed(unsigned long);
long secureRandom(long);
long secureRandom(long, long);
long map(long, long, long, long, long);

#endif

#endif

#endif
