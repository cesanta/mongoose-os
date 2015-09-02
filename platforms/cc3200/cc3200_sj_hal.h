#ifndef _CC3200_HAL_H_
#define _CC3200_HAL_H_

#include "v7.h"

#define PROMPT_CHAR_EVENT 0
#define V7_INVOKE_EVENT 1
struct prompt_event {
  int type;
  void *data;
};

struct v7_invoke_event_data {
  v7_val_t func;
  v7_val_t this_obj;
  v7_val_t args;
};

#endif /* _CC3200_HAL_H_ */
