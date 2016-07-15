/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_CONSOLE_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_CONSOLE_H_

#define SJ_CONSOLE_ENABLE_CLOUD \
  (!defined(DISABLE_C_CLUBBY) && !defined(CS_DISABLE_JS))

void cc3200_console_putc(int fd, char c);

#if SJ_CONSOLE_ENABLE_CLOUD
struct v7;
void sj_console_api_setup(struct v7 *v7);
void cc3200_console_cloud_push();
#endif

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_CONSOLE_H_ */
