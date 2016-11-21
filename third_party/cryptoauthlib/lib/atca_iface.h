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

#ifndef ATCA_IFACE_H
#define ATCA_IFACE_H

/** \defgroup interface ATCAIface (atca_)
 *  \brief Abstract interface to all CryptoAuth device types.  This interface
 *  connects to the HAL implementation and abstracts the physical details of the
 *  device communication from all the upper layers of CryptoAuthLib
   @{ */

#ifdef __cplusplus
extern "C" {
#endif

#include "atca_command.h"

typedef enum {
	ATCA_I2C_IFACE,
	ATCA_SWI_IFACE,
	ATCA_UART_IFACE,
	ATCA_SPI_IFACE,
	ATCA_HID_IFACE
	// additional physical interface types here
} ATCAIfaceType;

/* ATCAIfaceCfg is a mediator object between a completely abstract notion of a physical interface and an actual physical interface.

    The main purpose of it is to keep hardware specifics from bleeding into the higher levels - hardware specifics could include
    things like framework specific items (ASF SERCOM) vs a non-Atmel I2C library constant that defines an I2C port.   But I2C has
    roughly the same parameters regardless of architecture and framework.  I2C
 */

typedef struct {

	ATCAIfaceType iface_type;       // active iface - how to interpret the union below
	ATCADeviceType devtype;         // explicit device type

	union {                         // each instance of an iface cfg defines a single type of interface
		struct ATCAI2C {
			uint8_t slave_address;  // 8-bit slave address
			uint8_t bus;            // logical i2c bus number, 0-based - HAL will map this to a pin pair for SDA SCL
			uint32_t baud;          // typically 400000
		} atcai2c;

		struct ATCASWI {
			uint8_t bus;        // logical SWI bus - HAL will map this to a pin	or uart port
		} atcaswi;

		struct ATCAUART {
			int port;           // logic port number
			uint32_t baud;      // typically 115200
			uint8_t wordsize;   // usually 8
			uint8_t parity;     // 0 == even, 1 == odd, 2 == none
			uint8_t stopbits;   // 0,1,2
		} atcauart;

		struct ATCAHID {
			int idx;                // HID enumeration index
			uint32_t vid;           // Vendor ID of kit (0x03EB for CK101)
			uint32_t pid;           // Product ID of kit (0x2312 for CK101)
			uint32_t packetsize;    // Size of the USB packet
			uint8_t guid[16];       // The GUID for this HID device
		} atcahid;

	};

	uint16_t wake_delay;    // microseconds of tWHI + tWLO which varies based on chip type
	int rx_retries;         // the number of retries to attempt for receiving bytes
	void     *cfg_data;     // opaque data used by HAL in device discovery
} ATCAIfaceCfg;

typedef struct atca_iface * ATCAIface;
ATCAIface newATCAIface(ATCAIfaceCfg *cfg);  // constructor
// IFace methods
ATCA_STATUS atinit(ATCAIface caiface);
ATCA_STATUS atpostinit(ATCAIface caiface);
ATCA_STATUS atsend(ATCAIface caiface, uint8_t *txdata, int txlength);
ATCA_STATUS atreceive(ATCAIface caiface, uint8_t *rxdata, uint16_t *rxlength);
ATCA_STATUS atwake(ATCAIface caiface);
ATCA_STATUS atidle(ATCAIface caiface);
ATCA_STATUS atsleep(ATCAIface caiface);

// accessors
ATCAIfaceCfg * atgetifacecfg(ATCAIface caiface);
void* atgetifacehaldat(ATCAIface caiface);

void deleteATCAIface( ATCAIface *dev );      // destructor
/*---- end of OATCAIface ----*/

#ifdef __cplusplus
}
#endif
/** @} */
#endif



