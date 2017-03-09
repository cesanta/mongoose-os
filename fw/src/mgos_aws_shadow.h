/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_AWS_SHADOW_H_
#define CS_FW_SRC_MGOS_AWS_SHADOW_H_

#include <stdbool.h>
#include <stdint.h>

#include "common/mg_str.h"
#include "fw/src/mgos_features.h"
#include "fw/src/mgos_init.h"

#if MGOS_ENABLE_AWS_SHADOW && MGOS_ENABLE_MQTT

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_aws_shadow_event {
  MGOS_AWS_SHADOW_GET_ACCEPTED = 0,
  MGOS_AWS_SHADOW_GET_REJECTED = 1,
  MGOS_AWS_SHADOW_UPDATE_ACCEPTED = 2,
  MGOS_AWS_SHADOW_UPDATE_REJECTED = 3,
  MGOS_AWS_SHADOW_UPDATE_DELTA = 4,
};

/*
 * Main AWS Device Shadow state callback handler.
 * Will get invoked when new versions of the state arrive via one of the topics.
 * If true is returned, this version is recorded.
 * If false is returned, this version is discarded and may be presented again
 * in the future (e.g. through periodic GET).
 * For DELTA messages, state is passed as "desired", reported is not set.
 */
typedef bool (*mgos_aws_shadow_state_handler)(void *arg,
                                              enum mgos_aws_shadow_event ev,
                                              uint64_t version,
                                              const struct mg_str reported,
                                              const struct mg_str desired);
typedef void (*mgos_aws_shadow_error_handler)(void *arg,
                                              enum mgos_aws_shadow_event ev,
                                              int code, const char *message);

void mgos_aws_shadow_set_state_handler(mgos_aws_shadow_state_handler state_cb,
                                       void *arg);
void mgos_aws_shadow_set_error_handler(mgos_aws_shadow_error_handler state_cb,
                                       void *arg);

/*
 * Request shadow state. Response will arrive via GET_ACCEPTED topic.
 * Note that MGOS automatically does this on every (re)connect if
 * aws.shadow.get_on_connect is true (default).
 */
bool mgos_aws_shadow_get(void);

/*
 * Send an update. Format string should define the value of the "state" key,
 * i.e. it should be an object with "reported" and/or "desired" keys, e.g.:
 * `mgos_aws_shadow_updatef("{reported:{foo: %d, bar: %d}}", foo, bar)`.
 * Response will arrive via UPDATE_ACCEPTED or REJECTED topic.
 * If you want the update to be aplied only if a particular version is current,
 * specify the version. Otherwise set it to 0 to apply to any version.
 */
bool mgos_aws_shadow_updatef(uint64_t version, const char *state_jsonf, ...);

enum mgos_init_result mgos_aws_shadow_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_AWS_SHADOW && MGOS_ENABLE_MQTT */

#endif /* CS_FW_SRC_MGOS_AWS_SHADOW_H_ */
