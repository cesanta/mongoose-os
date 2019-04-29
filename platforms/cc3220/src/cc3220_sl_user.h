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
 * from ti/drivers/net/wifi/user.h
 * See that file for comments and docs.
 */

#ifndef CS_FW_PLATFORMS_CC3220_SRC_CC3220_SL_USER_H_
#define CS_FW_PLATFORMS_CC3220_SRC_CC3220_SL_USER_H_

#include <stdlib.h>
#include "cc32xx_sl_spawn.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <ti/drivers/net/wifi/porting/cc_pal.h>

typedef signed int _SlFd_t;

#define SL_TIMESTAMP_TICKS_IN_10_MILLISECONDS (_u32)(10)
#define SL_TIMESTAMP_MAX_VALUE (_u32)(0xFFFFFFFF)

#define MAX_CONCURRENT_ACTIONS 10

#define SL_RUNTIME_EVENT_REGISTERATION

#define SL_INC_ARG_CHECK
#define SL_INC_INTERNAL_ERRNO
#define SL_INC_EXT_API
#define SL_INC_WLAN_PKG
#define SL_INC_SOCKET_PKG
#define SL_INC_NET_APP_PKG
#define SL_INC_NET_CFG_PKG
#define SL_INC_NVMEM_PKG
#define SL_INC_NVMEM_EXT_PKG
#define SL_INC_SOCK_SERVER_SIDE_API
#define SL_INC_SOCK_CLIENT_SIDE_API
#define SL_INC_SOCK_RECV_API
#define SL_INC_SOCK_SEND_API

#define sl_DeviceEnablePreamble()
#define sl_DeviceEnable() NwpPowerOn()
#define sl_DeviceDisable() NwpPowerOff()

#define _SlFd_t Fd_t

#define sl_IfOpen spi_Open
#define sl_IfClose spi_Close
#define sl_IfRead spi_Read
#define sl_IfWrite spi_Write
#define sl_IfRegIntHdlr(InterruptHdl, pValue) \
  NwpRegisterInterruptHandler(InterruptHdl, pValue)
#define sl_IfMaskIntHdlr() NwpMaskInterrupt()
#define sl_IfUnMaskIntHdlr() NwpUnMaskInterrupt()
#define slcb_GetTimestamp TimerGetCurrentTimestamp
#define WAIT_NWP_SHUTDOWN_READY NwpWaitForShutDownInd()
#ifndef SL_INC_INTERNAL_ERRNO
#define slcb_SetErrno
#endif

#define SL_PLATFORM_MULTI_THREADED 1

#define SL_OS_RET_CODE_OK ((int) OS_OK)
#define SL_OS_WAIT_FOREVER ((uint32_t) OS_WAIT_FOREVER)
#define SL_OS_NO_WAIT ((uint32_t) OS_NO_WAIT)
#define _SlTime_t uint32_t

#define _SlSyncObj_t SemaphoreP_Handle
#define sl_SyncObjCreate(pSyncObj, pName) Semaphore_create_handle(pSyncObj)
#define sl_SyncObjDelete(pSyncObj) SemaphoreP_delete_handle(pSyncObj)
#define sl_SyncObjSignal(pSyncObj) SemaphoreP_post_handle(pSyncObj)
#define sl_SyncObjSignalFromIRQ(pSyncObj) SemaphoreP_post_handle(pSyncObj)
#define sl_SyncObjWait(pSyncObj, Timeout) \
  SemaphoreP_pend((*(pSyncObj)), Timeout)
#define _SlLockObj_t MutexP_Handle
#define sl_LockObjCreate(pLockObj, pName) Mutex_create_handle(pLockObj)
#define sl_LockObjDelete(pLockObj) MutexP_delete_handle(pLockObj)
#define sl_LockObjLock(pLockObj, Timeout) Mutex_lock(*(pLockObj))
#define sl_LockObjUnlock(pLockObj) Mutex_unlock(*(pLockObj))

#define SL_PLATFORM_EXTERNAL_SPAWN
#define sl_Spawn(pEntry, pValue, flags) cc32xx_sl_spawn(pEntry, pValue, flags)

#define SL_MEMORY_MGMT_DYNAMIC 1
#define SL_MEMORY_MGMT_STATIC 0
#define SL_MEMORY_MGMT SL_MEMORY_MGMT_DYNAMIC
#define sl_Malloc(Size) malloc(Size)
#define sl_Free(pMem) free(pMem)

#define slcb_DeviceFatalErrorEvtHdlr SimpleLinkFatalErrorEventHandler
//#define slcb_DeviceGeneralEvtHdlr		  SimpleLinkGeneralEventHandler
#define slcb_WlanEvtHdlr SimpleLinkWlanEventHandler
#define slcb_NetAppEvtHdlr SimpleLinkNetAppEventHandler
//#define slcb_NetAppHttpServerHdlr   SimpleLinkHttpServerEventHandler
//#define slcb_NetAppRequestHdlr  SimpleLinkNetAppRequestEventHandler
//#define slcb_NetAppRequestMemFree  SimpleLinkNetAppRequestMemFreeEventHandler
//#define slcb_SockEvtHdlr         SimpleLinkSockEventHandler

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif /* CS_FW_PLATFORMS_CC3220_SRC_CC3220_SL_USER_H_ */
