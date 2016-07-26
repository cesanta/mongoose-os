/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_CONSOLE_H_
#define CS_FW_SRC_SJ_CONSOLE_H_

void sj_console_init();

#ifdef SJ_ENABLE_CLUBBY
void sj_console_cloud_log(const char *fmt, ...);
int sj_console_is_waiting_for_resp();

#define CONSOLE_LOG(l, x)          \
  if (cs_log_level >= l) {         \
    cs_log_print_prefix(__func__); \
    cs_log_printf x;               \
    sj_console_cloud_log x;        \
  }
#else
#define CONSOLE_LOG(l, x) LOG(l, x)
#endif /* SJ_ENABLE_CLUBBY */

#endif /* CS_FW_SRC_SJ_CONSOLE_H_ */
