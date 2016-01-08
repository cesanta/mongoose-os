#ifndef SJ_CLUBBY_H
#define SJ_CLUBBY_H

#include "v7/v7.h"
#include "common/ubjserializer.h"
#include "clubby_proto.h"

#ifndef DISABLE_C_CLUBBY

typedef void (*clubby_callback_t)(struct clubby_event *evt, void *user_data);

void sj_init_clubby(struct v7 *v7);

void sj_clubby_send_reply(struct clubby_event *evt, int status,
                          const char *status_msg);

int sj_clubby_register_global_command(const char *cmd, clubby_callback_t cb,
                                      void *user_data);

/* TODO(alashkin): add more sending functions to header */
#endif /* DISABLE_C_CLUBBY */

#endif /* SJ_CLUBBY_H */
