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

#include <stdlib.h>
#include "atca_iface.h"
#include "hal/atca_hal.h"

/** \defgroup interface ATCAIface (atca_)
 *  \brief Abstract interface to all CryptoAuth device types.  This interface
 *  connects to the HAL implementation and abstracts the physical details of the
 *  device communication from all the upper layers of CryptoAuthLib
   @{ */

/** \brief atca_iface is the C object backing ATCAIface.  See the atca_iface.h file for
 * details on the ATCAIface methods
 */

struct atca_iface {
	ATCAIfaceType mType;
	ATCAIfaceCfg  *mIfaceCFG;   // points to previous defined/given Cfg object, caller manages this

	ATCA_STATUS (*atinit)(void *hal, ATCAIfaceCfg *);
	ATCA_STATUS (*atpostinit)(ATCAIface hal);
	ATCA_STATUS (*atsend)(ATCAIface hal, uint8_t *txdata, int txlength);
	ATCA_STATUS (*atreceive)( ATCAIface hal, uint8_t *rxdata, uint16_t *rxlength);
	ATCA_STATUS (*atwake)(ATCAIface hal);
	ATCA_STATUS (*atidle)(ATCAIface hal);
	ATCA_STATUS (*atsleep)(ATCAIface hal);

	// treat as private
	void *hal_data;     // generic pointer used by HAL to point to architecture specific structure
	                    // no ATCA object should touch this except HAL, HAL manages this pointer and memory it points to
};

ATCA_STATUS _atinit(ATCAIface caiface, ATCAHAL_t *hal);

/** \brief constructor for ATCAIface objects
 * \param[in] cfg  points to the logical configuration for the interface
 * \return ATCAIface
 */

ATCAIface newATCAIface(ATCAIfaceCfg *cfg)  // constructor
{
	ATCAIface caiface = (ATCAIface)malloc(sizeof(struct atca_iface));

	caiface->mType = cfg->iface_type;
	caiface->mIfaceCFG = cfg;

	if (atinit(caiface) != ATCA_SUCCESS) {
		free(caiface);
		caiface = NULL;
	}

	return caiface;
}

// public ATCAIface methods

ATCA_STATUS atinit(ATCAIface caiface)
{
	ATCA_STATUS status = ATCA_COMM_FAIL;
	ATCAHAL_t hal;

	_atinit( caiface, &hal );

	status = caiface->atinit( &hal, caiface->mIfaceCFG );
	if (status == ATCA_SUCCESS) {
		caiface->hal_data = hal.hal_data;

		// Perform the post init
		status = caiface->atpostinit( caiface );
	}

	return status;
}

ATCA_STATUS atsend(ATCAIface caiface, uint8_t *txdata, int txlength)
{
	return caiface->atsend(caiface, txdata, txlength);
}

ATCA_STATUS atreceive( ATCAIface caiface, uint8_t *rxdata, uint16_t *rxlength)
{
	return caiface->atreceive(caiface, rxdata, rxlength);
}

ATCA_STATUS atwake(ATCAIface caiface)
{
	return caiface->atwake(caiface);
}

ATCA_STATUS atidle(ATCAIface caiface)
{
	atca_delay_ms(1);
	return caiface->atidle(caiface);
}

ATCA_STATUS atsleep(ATCAIface caiface)
{
	atca_delay_ms(1);
	return caiface->atsleep(caiface);
}

ATCAIfaceCfg * atgetifacecfg(ATCAIface caiface)
{
	return caiface->mIfaceCFG;
}

void* atgetifacehaldat(ATCAIface caiface)
{
	return caiface->hal_data;
}

void deleteATCAIface(ATCAIface *caiface) // destructor
{
	if ( *caiface ) {
		hal_iface_release( (*caiface)->mType, (*caiface)->hal_data);  // let HAL clean up and disable physical level interface if ref count is 0
		free((void*)*caiface);
	}

	*caiface = NULL;
}

ATCA_STATUS _atinit(ATCAIface caiface, ATCAHAL_t *hal)
{
	// get method mapping to HAL methods for this interface
	hal_iface_init( caiface->mIfaceCFG, hal );
	caiface->atinit     = hal->halinit;
	caiface->atpostinit = hal->halpostinit;
	caiface->atsend     = hal->halsend;
	caiface->atreceive  = hal->halreceive;
	caiface->atwake     = hal->halwake;
	caiface->atsleep    = hal->halsleep;
	caiface->atidle     = hal->halidle;
	caiface->hal_data   = hal->hal_data;

	return ATCA_SUCCESS;
}
/** @} */
