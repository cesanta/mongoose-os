/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_CONSOLE_H_
#define CS_FW_SRC_SJ_CONSOLE_H_

struct v7;

#ifndef CS_DISABLE_JS
void sj_console_init(struct v7 *v7);
void sj_console_api_setup(struct v7 *v7);
#endif /* CS_DISABLE_JS */

void sj_console_cloud_log(const char *fmt, ...);
int sj_console_is_waiting_for_resp();

#define CONSOLE_LOG(l, x)          \
  if (cs_log_level >= l) {         \
    cs_log_print_prefix(__func__); \
    cs_log_printf x;               \
    sj_console_cloud_log x;        \
  }

#endif /* CS_FW_SRC_SJ_CONSOLE_H_ */
