/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_CLUBBY_CHANNEL_H_
#define CS_FW_SRC_MG_CLUBBY_CHANNEL_H_

#include <inttypes.h>
#include <stdbool.h>

#include "common/mg_str.h"

#include "fw/src/sj_sys_config.h"

#ifdef SJ_ENABLE_CLUBBY

enum mg_clubby_channel_event {
  MG_CLUBBY_CHANNEL_OPEN,
  MG_CLUBBY_CHANNEL_FRAME_RECD,
  MG_CLUBBY_CHANNEL_FRAME_SENT,
  MG_CLUBBY_CHANNEL_CLOSED,
};

struct mg_clubby_channel {
  void (*ev_handler)(struct mg_clubby_channel *ch,
                     enum mg_clubby_channel_event ev, void *ev_data);
  void (*connect)(struct mg_clubby_channel *ch);
  bool (*send_frame)(struct mg_clubby_channel *ch, const struct mg_str f);
  void (*close)(struct mg_clubby_channel *ch);
  const char *(*get_type)(struct mg_clubby_channel *ch);
  bool (*is_persistent)(struct mg_clubby_channel *ch);

  void *channel_data;
  void *clubby_data;
  void *user_data;
};

#endif /* SJ_ENABLE_CLUBBY */
#endif /* CS_FW_SRC_MG_CLUBBY_CHANNEL_H_ */
