/**
 * \file
 * \brief a set of default configurations for various ATCA devices and interfaces
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

#include <stddef.h>
#include "atca_cfgs.h"
#include "atca_iface.h"
#include "atca_device.h"

/** \defgroup config Configuration (cfg_)
 * \brief Logical device configurations describe the CryptoAuth device type and logical interface.
   @{ */

/* if the number of these configurations grows large, we can #ifdef them based on required device support */

/** \brief default configuration for an ECCx08A device */
ATCAIfaceCfg cfg_ateccx08a_i2c_default = {
	.iface_type				= ATCA_I2C_IFACE,
	.devtype				= ATECC508A,
	.atcai2c.slave_address	= 0xC0,
	.atcai2c.bus			= 2,
	.atcai2c.baud			= 400000,
	//.atcai2c.baud = 100000,
	.wake_delay				= 800,
	.rx_retries				= 20
};

/** \brief default configuration for an ECCx08A device on the logical SWI bus over UART*/
ATCAIfaceCfg cfg_ateccx08a_swi_default = {
	.iface_type		= ATCA_SWI_IFACE,
	.devtype		= ATECC508A,
	.atcaswi.bus	= 4,
	.wake_delay		= 800,
	.rx_retries		= 10
};

/** \brief default configuration for a SHA204A device on the first logical I2C bus */
ATCAIfaceCfg cfg_sha204a_i2c_default = {
	.iface_type				= ATCA_I2C_IFACE,
	.devtype				= ATSHA204A,
	.atcai2c.slave_address	= 0xC8,
	.atcai2c.bus			= 2,
	.atcai2c.baud			= 400000,
	.wake_delay				= 2560,
	.rx_retries				= 20
};

/** \brief default configuration for an SHA204A device on the logical SWI bus over UART*/
ATCAIfaceCfg cfg_sha204a_swi_default = {
	.iface_type		= ATCA_SWI_IFACE,
	.devtype		= ATSHA204A,
	.atcaswi.bus	= 4,
	.wake_delay		= 2560,
	.rx_retries		= 10
};

/** \brief default configuration for Kit protocol over the device's async interface */
ATCAIfaceCfg cfg_ecc508_kitcdc_default = {
	.iface_type			= ATCA_UART_IFACE,
	.devtype			= ATECC508A,
	.atcauart.port		= 0,
	.atcauart.baud		= 115200,
	.atcauart.wordsize	= 8,
	.atcauart.parity	= 2,
	.atcauart.stopbits	= 1,
	.rx_retries			= 1,
};

/** \brief default configuration for Kit protocol over the device's async interface */
ATCAIfaceCfg cfg_ecc508_kithid_default = {
	.iface_type			= ATCA_HID_IFACE,
	.devtype			= ATECC508A,
	.atcahid.idx		= 0,
	.atcahid.vid		= 0x03EB,
	.atcahid.pid		= 0x2312,
	.atcahid.packetsize = 64,
	.atcahid.guid		= { 0x4d,		 0x1e, 0x55, 0xb2, 0xf1, 0x6f, 0x11, 0xcf, 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 },
};

/** \brief default configuration for Kit protocol over the device's async interface */
ATCAIfaceCfg cfg_sha204_kithid_default = {
	.iface_type			= ATCA_HID_IFACE,
	.devtype			= ATSHA204A,
	.atcahid.idx		= 0,
	.atcahid.vid		= 0x03EB,
	.atcahid.pid		= 0x2312,
	.atcahid.packetsize = 64,
	.atcahid.guid		= { 0x4d,		 0x1e, 0x55, 0xb2, 0xf1, 0x6f, 0x11, 0xcf, 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 },
};

/** @} */
