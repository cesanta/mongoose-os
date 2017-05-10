/**
 * \file
 * \brief Single aggregation point for all CryptoAuthLib header files
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

#ifndef _ATCA_LIB_H
#define _ATCA_LIB_H

#include <stddef.h>
#include <string.h>

#include "hal/atca_hal.h"
#include "atca_status.h"
#include "atca_device.h"
#include "atca_command.h"
#include "atca_cfgs.h"
#include "basic/atca_basic.h"
#include "basic/atca_helpers.h"

#ifdef ATCAPRINTF
	#include <stdio.h>
	//#define BREAK(status, message) {printf(__FUNCTION__": "message" -- Status: %02X\r\n", status); break;}
	#define BREAK(status, message) {printf(": " message " -- Status: %02X\r\n", status); break;}
	#define RETURN(status, message) {printf(": " message " -- Status: %02X\r\n", status); return status;}
	#define PRINTSTAT(status, message) {printf(": " message " -- Status: %02X\r\n", status);}
	#define PRINT(message) {printf(": " message "\r\n"); break;}
	#define DBGOUT(message) {printf(": " message "\r\n"); break;}
#else
	#define BREAK(status, message) {break;}
	#define RETURN(status, message) {return status;}
	#define PRINT(message) {break;}
	#define DBGOUT(message) {break;}
#endif

#endif
