#ifndef SJ_CLUBBY_H
#define SJ_CLUBBY_H

#include "v7/v7.h"
#include "common/ubjserializer.h"

#ifndef DISABLE_C_CLUBBY

void sj_init_clubby(struct v7 *v7);

void sj_clubby_connect();
void sj_clubby_disconnect();
void sj_clubby_send_resp(const char *dst, int64_t id, int status,
                         const char *status_msg);

/* TODO(alashkin): add more sending functions to header */
#endif /* DISABLE_C_CLUBBY */

#endif /* SJ_CLUBBY_H */
