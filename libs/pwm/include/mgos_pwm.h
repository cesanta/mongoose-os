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

#ifndef CS_MOS_LIBS_PWM_SRC_MGOS_PWM_H_
#define CS_MOS_LIBS_PWM_SRC_MGOS_PWM_H_

#include <stdbool.h>

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
 *  duty:   Specifies which fraction of the cycle is spent in "1" state:
 *          0 is always off, 0.5 is a square wave, 1 is always on.
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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_PWM_SRC_MGOS_PWM_H_ */
