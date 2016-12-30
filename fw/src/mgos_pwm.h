/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_PWM_H_
#define CS_FW_SRC_MGOS_PWM_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int mgos_pwm_set(int pin, int period, int duty);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_PWM_H_ */
