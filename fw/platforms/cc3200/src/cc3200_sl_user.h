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
 * This file configures the SimpleLink subsystem and is originally derived
 * from cc3200-sdk/simplelink/user.h
 * See that file for comments and docs.
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_SL_USER_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_SL_USER_H_

#include <stdlib.h>
#include "cc32xx_sl_spawn.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "cc_pal.h"

/* static installation of the provisioning extarnal library*/
#include "provisioning_api.h"
//#define SL_EXT_LIB_1
// sl_Provisioning

#define SL_TIMESTAMP_TICKS_IN_10_MILLISECONDS (_u32)(10000000)
#define SL_TIMESTAMP_MAX_VALUE 0xFFFFFFFF /* 32-bit timer counter */

#define MAX_CONCURRENT_ACTIONS 10
#define CPU_FREQ_IN_MHZ 80
#define SL_INC_ARG_CHECK
//#define SL_INC_STD_BSD_API_NAMING
#define SL_INC_EXT_API
#define SL_INC_WLAN_PKG
#define SL_INC_SOCKET_PKG
#define SL_INC_NET_APP_PKG
#define SL_INC_NET_CFG_PKG
#define SL_INC_NVMEM_PKG
#define SL_INC_SOCK_SERVER_SIDE_API
#define SL_INC_SOCK_CLIENT_SIDE_API
#define SL_INC_SOCK_RECV_API
#define SL_INC_SOCK_SEND_API

#define sl_DeviceEnablePreamble() NwpPowerOnPreamble()
#define sl_DeviceEnable() NwpPowerOn()
#define sl_DeviceDisable() NwpPowerOff()
#define sl_DeviceDisable_WithNwpLpdsPoll() NwpPowerOff_WithNwpLpdsPoll()
#define sl_DeviceDisablePreamble() NwpPrePowerOffTimout0()
#define _SlFd_t Fd_t
#define sl_IfOpen spi_Open
#define sl_IfClose spi_Close
#define sl_IfRead spi_Read
#define sl_IfWrite spi_Write
#define sl_IfRegIntHdlr(InterruptHdl, pValue) \
  NwpRegisterInterruptHandler(InterruptHdl, pValue)
#define sl_IfMaskIntHdlr() NwpMaskInterrupt()
#define sl_IfUnMaskIntHdlr() NwpUnMaskInterrupt()
/* #define SL_START_WRITE_STAT */
#define sl_GetTimestamp TimerGetCurrentTimestamp

#define SL_PLATFORM_MULTI_THREADED

#ifdef MGOS_HAVE_FREERTOS
#include "osi.h"
#define SL_OS_RET_CODE_OK ((int) OSI_OK)
#define SL_OS_WAIT_FOREVER ((OsiTime_t) OSI_WAIT_FOREVER)
#define SL_OS_NO_WAIT ((OsiTime_t) OSI_NO_WAIT)
#define _SlTime_t OsiTime_t
typedef OsiSyncObj_t _SlSyncObj_t;
#define sl_SyncObjCreate(pSyncObj, pName) osi_SyncObjCreate(pSyncObj)
#define sl_SyncObjDelete(pSyncObj) osi_SyncObjDelete(pSyncObj)
#define sl_SyncObjSignal(pSyncObj) osi_SyncObjSignal(pSyncObj)
#define sl_SyncObjSignalFromIRQ(pSyncObj) osi_SyncObjSignalFromISR(pSyncObj)
#define sl_SyncObjWait(pSyncObj, Timeout) osi_SyncObjWait(pSyncObj, Timeout)
typedef OsiLockObj_t _SlLockObj_t;
#define sl_LockObjCreate(pLockObj, pName) osi_LockObjCreate(pLockObj)
#define sl_LockObjDelete(pLockObj) osi_LockObjDelete(pLockObj)
#define sl_LockObjLock(pLockObj, Timeout) osi_LockObjLock(pLockObj, Timeout)
#define sl_LockObjUnlock(pLockObj) osi_LockObjUnlock(pLockObj)
#endif /* MGOS_HAVE_FREERTOS */

#define SL_PLATFORM_EXTERNAL_SPAWN
#define sl_Spawn(pEntry, pValue, flags) cc32xx_sl_spawn(pEntry, pValue, flags)

#define SL_MEMORY_MGMT_DYNAMIC 1
#define SL_MEMORY_MGMT_STATIC 0

#define SL_MEMORY_MGMT SL_MEMORY_MGMT_DYNAMIC

#define sl_Malloc(Size) malloc(Size)
#define sl_Free(pMem) free(pMem)

#define _SlDrvHandleGeneralEvents SimpleLinkGeneralEventHandler
#define sl_WlanEvtHdlr SimpleLinkWlanEventHandler
#define sl_NetAppEvtHdlr SimpleLinkNetAppEventHandler
//#define sl_HttpServerCallback SimpleLinkHttpServerCallback
//#define sl_SockEvtHdlr SimpleLinkSockEventHandler

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_SL_USER_H_ */
