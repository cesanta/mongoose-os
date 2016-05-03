/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef SJ_CONSOLE_H
#define SJ_CONSOLE_H

#ifndef CS_DISABLE_JS

struct v7;

void sj_console_init(struct v7 *v7);
void sj_console_api_setup(struct v7 *v7);

#endif /* CS_DISABLE_JS */

#endif /* SJ_CONSOLE_H */
