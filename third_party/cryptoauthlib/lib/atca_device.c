/**
 * \file
 * \brief  Atmel CryptoAuth device object
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

#include <stdlib.h>
#include "atca_device.h"

/** \defgroup device ATCADevice (atca_)
 * \brief ATCADevice object - composite of command and interface objects
   @{ */

/** \brief atca_device is the C object backing ATCADevice.  See the atca_device.h file for
 * details on the ATCADevice methods
 */

struct atca_device {
	ATCACommand mCommands;  // has-a command set to support a given CryptoAuth device
	ATCAIface mIface;       // has-a physical interface
};

/** \brief constructor for an Atmel CryptoAuth device
 * \param[in] cfg  pointer to an interface configuration object
 * \return reference to a new ATCADevice
 */

ATCADevice newATCADevice(ATCAIfaceCfg *cfg )
{
	ATCADevice cadev = NULL;

	if (cfg == NULL)
		return NULL;

	cadev = (ATCADevice)malloc(sizeof(struct atca_device));
	cadev->mCommands = (ATCACommand)newATCACommand(cfg->devtype);
	cadev->mIface    = (ATCAIface)newATCAIface(cfg);

	if (cadev->mCommands == NULL || cadev->mIface == NULL) {
		free(cadev);
		cadev = NULL;
	}

	return cadev;
}

/** \brief returns a reference to the ATCACommand object for the device
 * \param[in] dev  reference to a device
 * \return reference to the ATCACommand object for the device
 */
ATCACommand atGetCommands( ATCADevice dev )
{
	return dev->mCommands;
}

/** \brief returns a reference to the ATCAIface interface object for the device
 * \param[in] dev  reference to a device
 * \return reference to the ATCAIface object for the device
 */

ATCAIface atGetIFace( ATCADevice dev )
{
	return dev->mIface;
}

/** \brief destructor for a device NULLs reference after object is freed
 * \param[in] cadev  pointer to a reference to a device
 *
 */

void deleteATCADevice( ATCADevice *cadev ) // destructor
{
	struct atca_device *dev = *cadev;

	if ( *cadev ) {
		deleteATCACommand( (ATCACommand*)&(dev->mCommands));
		deleteATCAIface((ATCAIface*)&(dev->mIface));
		free((void*)*cadev);
	}

	*cadev = NULL;
}

/** @} */
