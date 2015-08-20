#ifndef _CC3200_HAL_H_
#define _CC3200_HAL_H_

#include "v7.h"

#define PROMPT_CHAR_EVENT 0
#define V7_EXEC_EVENT 1
struct prompt_event {
  int type;
  void *data;
};

struct v7_exec_event_data {
  char *code;
  v7_val_t this_obj;
};

#endif /* _CC3200_HAL_H_ */
