/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

// Note: this file overrides the file shipped with the SDK.

#pragma once  // no_extern_c_check

// Netowrking is provided by LwIP.
#define RSI_NUMBER_OF_SOCKETS 0

#ifndef RSI_WLAN_TX_POOL_PKT_COUNT
#define RSI_WLAN_TX_POOL_PKT_COUNT 1
#endif

#ifndef RSI_BT_COMMON_TX_POOL_PKT_COUNT
#define RSI_BT_COMMON_TX_POOL_PKT_COUNT 1
#endif

#ifndef RSI_BT_CLASSIC_TX_POOL_PKT_COUNT
#define RSI_BT_CLASSIC_TX_POOL_PKT_COUNT 1
#endif

#ifdef RSI_BLE_TX_POOL_PKT_COUNT
#define RSI_BLE_TX_POOL_PKT_COUNT 1
#endif

#ifndef RSI_ZIGB_TX_POOL_PKT_COUNT
#define RSI_ZIGB_TX_POOL_PKT_COUNT 1
#endif

#ifndef RSI_COMMON_TX_POOL_PKT_COUNT
#define RSI_COMMON_TX_POOL_PKT_COUNT 1
#endif

#ifndef RSI_DRIVER_RX_POOL_PKT_COUNT
#define RSI_DRIVER_RX_POOL_PKT_COUNT 1
#endif

#define RSI_LITTLE_ENDIAN 1
