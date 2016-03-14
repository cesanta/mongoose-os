/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_PLATFORMS_CC3200_CC3200_LEDS_H_
#define CS_SMARTJS_PLATFORMS_CC3200_CC3200_LEDS_H_

/* A handy way to control LEDs on the LaunchXL. */

/* Configures for output the pins to which LEDs are attached.
 * Turns off all LEDs. */
void cc3200_leds_init();

enum cc3200_led { RED = 2, AMBER = 4, GREEN = 8 };
enum cc3200_led_state { OFF = 0, ON = 1, TOGGLE = 2 };
void cc3200_leds(enum cc3200_led led, enum cc3200_led_state s);

#endif /* CS_SMARTJS_PLATFORMS_CC3200_CC3200_LEDS_H_ */
