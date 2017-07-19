/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

/*
 * See on GitHub:
 * [mgos_pwm.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_pwm.h)
 * [mgos_pwm.c](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_pwm.c)
 * [esp32_pwm.c](https://github.com/cesanta/mongoose-os/blob/master/fw/platforms/src/esp32/esp32_pwm.c)
 * [esp_pwm.c](https://github.com/cesanta/mongoose-os/blob/master/fw/platforms/src/esp8266/esp_pwm.c)
 */

#ifndef CS_FW_SRC_MGOS_PWM_H_
#define CS_FW_SRC_MGOS_PWM_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 *
 *  Set and control the PWM.
 *
 *  Params:
 *  pin:    GPIO number.
 *  freq:   Frequency, in Hz. 0 disables PWM on the pin.
 *  duty:   Duty, in %, how long to spend in "1" state. Must be between 0 and
 *          100 (inclusive). 0 is "always off", 100 is "always on",
 *          50 is a square wave.
 *  Return:
 *  true - SUCCESS, false - FAIL.
 *
 *  Note:
 *  On esp32 we use 8 channels and 4 timers.
 *  Each `mgos_set_pwm` call with new pin number assigns a new channel.
 *  If we already have timer running at the specified frequency,
 *  we use it instead of assigning a new one.
 */
bool mgos_pwm_set(int pin, int freq, float duty);

/*
 *  It's a wrapper for correct using a float param by ffi'g.
 */
bool mgos_pwm_set_double(int pin, int freq, double duty);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_PWM_H_ */
