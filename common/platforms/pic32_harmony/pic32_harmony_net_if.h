/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PIC32_HARMONY_NET_IF_H_
#define CS_COMMON_PLATFORMS_PIC32_HARMONY_NET_IF_H_

#include "mongoose/src/net_if.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MG_ENABLE_NET_IF_PIC32_HARMONY
#define MG_ENABLE_NET_IF_PIC32_HARMONY MG_NET_IF == MG_NET_IF_PIC32_HARMONY
#endif

extern struct mg_iface_vtable mg_pic32_harmony_iface_vtable;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_PLATFORMS_PIC32_HARMONY_NET_IF_H_ */
