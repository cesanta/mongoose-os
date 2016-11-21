/**
 * \file
 *
 * \brief  Atmel Crypto Auth hardware interface object
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \atmel_crypto_device_library_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel integrated circuit.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \atmel_crypto_device_library_license_stop
 */

#ifndef KIT_PROTOCOL_H
#define KIT_PROTOCOL_H

#include "cryptoauthlib.h"

// Define this for debugging communication
//#define KIT_DEBUG

/** \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 *
   @{ */

// The number of bytes to wrap a command in kit protocol.  sizeof("s:t()\n<null>")
#define KIT_TX_WRAP_SIZE    (7)

// The number of bytes to wrap a response in kit protocol.  sizeof("<KIT_MSG_SIZE>00()\n<null>")
#define KIT_MSG_SIZE        (32)
#define KIT_RX_WRAP_SIZE    (KIT_MSG_SIZE + 6)

#ifdef __cplusplus
extern "C" {
#endif

ATCA_STATUS kit_init(ATCAIface iface);

ATCA_STATUS kit_send(ATCAIface iface, uint8_t* txdata, int txlength);
ATCA_STATUS kit_receive(ATCAIface iface, uint8_t* rxdata, uint16_t* rxsize);

ATCA_STATUS kit_wrap_cmd(uint8_t* txdata, int txlength, char* pkitbuf, int* nkitbuf);
ATCA_STATUS kit_parse_rsp(char* pkitbuf, int nkitbuf, uint8_t* kitstatus, uint8_t* rxdata, int* nrxdata);

ATCA_STATUS kit_wake(ATCAIface iface);
ATCA_STATUS kit_idle(ATCAIface iface);
ATCA_STATUS kit_sleep(ATCAIface iface);

#ifdef __cplusplus
}
#endif

/** @} */

#endif // KIT_PROTOCOL_H
