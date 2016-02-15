/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef _SJ_PWM_H_INCLUDED
#define _SJ_PWM_H_INCLUDED

struct v7;

int sj_pwm_set(int pin, int period, int duty);

#endif /* _SJ_PWM_H_INCLUDED */
