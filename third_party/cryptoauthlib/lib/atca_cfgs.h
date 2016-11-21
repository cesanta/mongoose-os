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


#ifndef ATCA_CFGS_H_
#define ATCA_CFGS_H_

#include "atca_iface.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \brief default configuration for an ECCx08A device on the first logical I2C bus */
extern ATCAIfaceCfg cfg_ateccx08a_i2c_default;

/** \brief default configuration for an ECCx08A device on the logical SWI bus over UART*/
extern ATCAIfaceCfg cfg_ateccx08a_swi_default;

/** \brief default configuration for a SHA204A device on the first logical I2C bus */
extern ATCAIfaceCfg cfg_sha204a_i2c_default;

/** \brief default configuration for an SHA204A device on the logical SWI bus over UART*/
extern ATCAIfaceCfg cfg_sha204a_swi_default;

/** \brief default configuration for Kit protocol over a CDC interface */
extern ATCAIfaceCfg cfg_ecc508_kitcdc_default;

/** \brief default configuration for Kit protocol over a HID interface */
extern ATCAIfaceCfg cfg_ecc508_kithid_default;

/** \brief default configuration for Kit protocol over a HID interface for SHA204 */
extern ATCAIfaceCfg cfg_sha204_kithid_default;

/** \brief example of a default configuration for AES132 SPI */
extern ATCAIfaceCfg cfg_ataes132_spi_default;

#ifdef __cplusplus
}
#endif
#endif /* ATCA_CFGS_H_ */