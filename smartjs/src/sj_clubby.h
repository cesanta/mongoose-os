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

/*
 * Sends an array of clubby commands.
 * cmds parameter must be an ubjson array (created by `ub_create_array`)
 * and each element should represent one command (created by `ub_create_object`)
 */
void sj_clubby_send(struct ub_ctx *ctx, const char *dst, ub_val_t cmds);

#endif /* DISABLE_C_CLUBBY */

#endif /* SJ_CLUBBY_H */
