/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * AWS GreenGrass API.
 *
 * There is not much here, because GreenGrass support is almost transparent.
 * Enable GG in the configuration `aws.greengrass.enable=true` and magically
 * the global MQTT connection goes to GG instead of AWS IoT.
 * GG core bootstrapping is done transparently by the library.
 */

#ifndef CS_MOS_LIBS_AWS_SRC_MGOS_AWS_GREENGRASS_H_
#define CS_MOS_LIBS_AWS_SRC_MGOS_AWS_GREENGRASS_H_

#include <mgos_net.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MG_ENABLE_SSL

/* Network configuration hook handler for the AWS GreenGrass. */
void aws_gg_net_ready(int ev, void *evd, void *arg);

/* Reconnect to GreenGrass. */
void aws_gg_reconnect(void);

#endif /* MG_ENABLE_SSL */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_AWS_SRC_MGOS_AWS_GREENGRASS_H_ */
