/**
 * \file
 * \brief ATCA Hardware abstraction layer for I2C bit banging.
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

#include "cryptoauthlib.h"
#include "atca_hal.h"
#include "hal_avr_kit_i2c.h"
#include "i2c_phys.h"
#include "timers.h"

/**
 * \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief These methods define the hardware abstraction layer for
 *        communicating with a CryptoAuth device using I2C bit banging.
   @{ */

void hal_idle_timer_stop(void)
{
	TIMSK0 &= ~_BV(TOIE0);  //Disable TC0 overflow interrupt.
	TCCR0B = 0x00;          //Disable clock source. Timer stops.
	TIFR0 |= _BV(TOV0);     //Clear interrupt flag for TC0 overflow.
	TIMSK0 |= _BV(TOIE0);   //Enable TC0 overflow interrupt.
}

/**
 * \brief This function creates a Start condition and sends the I2C
 *        address.
 *
 * \param[in] RorW  I2C_READ for reading, I2C_WRITE for writing.
 *
 * \return ATCA_STATUS
 */
static ATCA_STATUS hal_i2c_send_slave_address(ATCAIface iface, uint8_t RorW)
{
	ATCA_STATUS status = ATCA_COMM_FAIL;
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	uint8_t sla = cfg->atcai2c.slave_address | RorW;

	status = i2c_send_start();
	if (status != ATCA_SUCCESS)
		return status;

	status = i2c_send_bytes(1, &sla);
	if (status != ATCA_SUCCESS)
		(void) i2c_send_stop();

	return status;
}

/**
 * \brief hal_i2c_init manages requests to initialize a physical
 *        interface. It manages use counts so when an interface has
 *        released the physical layer, it will disable the interface for
 *        some other use. You can have multiple ATCAIFace instances using
 *        the same bus, and you can have multiple ATCAIFace instances on
 *        multiple i2c buses, so hal_i2c_init manages these things and
 *        ATCAIFace is abstracted from the physical details.
 */

/**
 * \brief Initialize an I2C interface using given config.
 *
 * \param[in] hal  opaque pointer to HAL data
 * \param[in] cfg  interface configuration
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_init(void *hal, ATCAIfaceCfg *cfg)
{
	i2c_enable();
	return ATCA_SUCCESS;
}

/**
 * \brief HAL implementation of I2C post init.
 *
 * \param[in] iface  ATCAIface instance
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_post_init(ATCAIface iface)
{
	return ATCA_SUCCESS;
}

/**
 * \brief Send byte(s) via I2C.
 *
 * \param[in] iface     interface of the logical device to send data to
 * \param[in] txdata    pointer to bytes to send
 * \param[in] txlength  number of bytes to send
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t *txdata, int txlength)
{
	ATCA_STATUS status = ATCA_TX_TIMEOUT;

	txdata[0] = 0x03;	//!< Word Address Value = Command
	txlength++;			//!< count Word Address byte towards txlength

	do {
		//! Address the device and indicate that bytes are to be written
		status = hal_i2c_send_slave_address(iface, I2C_WRITE);
		if (status != ATCA_SUCCESS)
			break;

		status = i2c_send_bytes(txlength, txdata);

	} while(0);

	//! Send STOP regardless of i2c_status	
	i2c_send_stop();

	return status;
}

/**
 * \brief Receive byte(s) via I2C.
 *
 * \param[in] iface     interface of the logical device to receive data
 *                      from
 * \param[in] rxdata    pointer to where bytes will be received
 * \param[in] rxlength  pointer to expected number of receive bytes to
 *                      request
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_receive(ATCAIface iface, uint8_t *rxdata, uint16_t *rxlength)
{
	ATCA_STATUS status = ATCA_RX_TIMEOUT;
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	int retries = cfg->rx_retries;
	uint8_t count;

	while (retries-- > 0 && status != ATCA_SUCCESS) {
		//! Address the device and indicate that bytes are to be read
		status = hal_i2c_send_slave_address(iface, I2C_READ);
		if (status == ATCA_SUCCESS) {
			//! Receive count byte
			i2c_receive_byte(rxdata);
			count = rxdata[0];
			if ((count < ATCA_RSP_SIZE_MIN) || (count > *rxlength)) {
				i2c_send_stop();
				status = ATCA_INVALID_SIZE;
				break;
			}

			//! Receive the remaining bytes
			i2c_receive_bytes(count - 1, &rxdata[1]);
		}
	}
	if (status == ATCA_TX_TIMEOUT)
		status = ATCA_RX_NO_RESPONSE;

	return status;
}

/**
 * \brief Send Wake flag via I2C.
 *
 * \param[in] iface  interface of the logical device to wake up
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_wake(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);

	hal_idle_timer_stop();
	// Generate wakeup pulse by disabling the I2C peripheral and
	// pulling SDA low. The I2C peripheral gets automatically
	// re-enabled when calling i2c_send_start().
	TWCR = 0;           // Disable I2C.
	DDRD |= _BV(PD1);   // Set SDA as output.
	PORTD &= ~_BV(PD1); // Set SDA low.
	// Debug Diamond: Increase wake-up pulse to 120 us.
	delay_10us(12);
	PORTD |= _BV(PD1);  // Set SDA high.
	delay_10us(250);

	return ATCA_SUCCESS;
}

/**
 * \brief Send Idle flag via I2C.
 *
 * \param[in] iface  interface of the logical device to idle
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_idle(ATCAIface iface)
{
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	ATCA_STATUS status = ATCA_TX_TIMEOUT;
	uint8_t data = 0x02;

	status = hal_i2c_send_slave_address(iface, I2C_WRITE);
	if (status != ATCA_SUCCESS) {
		(void) i2c_send_stop();
		return ATCA_COMM_FAIL;
	}

	status = i2c_send_bytes(1, &data);
	if (status != ATCA_SUCCESS) {
		return ATCA_COMM_FAIL;
	} else {
		(void) i2c_send_stop();
		return ATCA_SUCCESS;
	}
}

/**
 * \brief Send Sleep flag via I2C.
 *
 * \param[in] iface  interface of the logical device to sleep
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_sleep(ATCAIface iface)
{
	ATCA_STATUS status = ATCA_TX_TIMEOUT;
	ATCAIfaceCfg *cfg = atgetifacecfg(iface);
	uint8_t data = 0x01;

	status = hal_i2c_send_slave_address(iface, I2C_WRITE);
	if (status != ATCA_SUCCESS) {
		(void) i2c_send_stop();
		return ATCA_COMM_FAIL;
	}

	status = i2c_send_bytes(1, &data);
	if (status != ATCA_SUCCESS) {
		return ATCA_COMM_FAIL;
	} else {
		(void) i2c_send_stop();
		return ATCA_SUCCESS;
	}
}

/**
 * \brief Manages reference count on given bus and releases resource if
 *        no more reference(s) exist.
 *
 * \param[in] hal_data  opaque pointer to hal data structure - known only
 *                      to the HAL implementation
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS hal_i2c_release(void *hal_data)
{
	i2c_disable();
	return ATCA_SUCCESS;	
}

/** @} */
