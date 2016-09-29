/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CLUBBY_CLUBBY_CHANNEL_H_
#define CS_COMMON_CLUBBY_CLUBBY_CHANNEL_H_

#include <inttypes.h>
#include <stdbool.h>

#include "common/mg_str.h"

#ifdef MG_ENABLE_CLUBBY

enum clubby_channel_event {
  MG_CLUBBY_CHANNEL_OPEN,
  MG_CLUBBY_CHANNEL_FRAME_RECD,
  MG_CLUBBY_CHANNEL_FRAME_SENT,
  MG_CLUBBY_CHANNEL_CLOSED,
};

struct clubby_channel {
  void (*ev_handler)(struct clubby_channel *ch, enum clubby_channel_event ev,
                     void *ev_data);
  void (*connect)(struct clubby_channel *ch);
  bool (*send_frame)(struct clubby_channel *ch, const struct mg_str f);
  void (*close)(struct clubby_channel *ch);
  const char *(*get_type)(struct clubby_channel *ch);
  bool (*is_persistent)(struct clubby_channel *ch);

  void *channel_data;
  void *clubby_data;
  void *user_data;
};

#endif /* MG_ENABLE_CLUBBY */
#endif /* CS_COMMON_CLUBBY_CLUBBY_CHANNEL_H_ */
