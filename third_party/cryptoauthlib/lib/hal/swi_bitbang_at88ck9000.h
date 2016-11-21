/**
 * \file
 * \brief  definitions for bit-banged SWI
 * \author Atmel Crypto Products
 * \date   November 18, 2015
 *
 * \copyright Copyright (c) 2015 Atmel Corporation. All rights reserved.
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

#ifndef SWI_BITBANG_AT88CK9000_H_
#define SWI_BITBANG_AT88CK9000_H_

#include "atca_status.h"
#include "timer_utilities.h"


#define MAX_SWI_BUSES   10      //!< AT88CK9000 has 10 sets of GPIO pin dedicated for SWI


typedef struct {
	uint32_t pin_sda[MAX_SWI_BUSES];
} SWIBuses;

extern SWIBuses swi_buses_default;


/**
 * \name Macros for Bit-Banged SWI Timing
 *
 * Times to drive bits at 230.4 kbps.
   @{ */

//! delay macro for width of one pulse (start pulse or zero pulse)
//! should be 4.34 us, is 4.67 us
#define BIT_DELAY_1L        delay_us(3)
//! should be 4.34 us, is 4.42 us
#define BIT_DELAY_1H        delay_us(3)

//! time to keep pin high for five pulses plus stop bit (used to bit-bang CryptoAuth 'zero' bit)
//! should be 26.04 us, is 26.42 us
#define BIT_DELAY_5        delay_us(24)    // considering pin set delay

//! time to keep pin high for seven bits plus stop bit (used to bit-bang CryptoAuth 'one' bit)
//! should be 34.72 us, is 34.75 us
#define BIT_DELAY_7        delay_us(32)    // considering pin set delay

//! turn around time when switching from receive to transmit
//! should be 15 us, is 15.4 us
#define RX_TX_DELAY         delay_us(14)

//! One loop iteration for edge detection takes about 0.6 us on this hardware.
//! Lets set the timeout value for start pulse detection to the uint8_t maximum.
//! This value is decremented while waiting for the falling edge of a start pulse.
#define START_PULSE_TIME_OUT    (255)

//! We measured a loop count of 8 for the start pulse. That means it takes about
//! 0.6 us per loop iteration. Maximum time between rising edge of start pulse
//! and falling edge of zero pulse is 8.6 us. Therefore, a value of 26 (around 15 us)
//! gives ample time to detect a zero pulse and also leaves enough time to detect
//! the following start pulse.
//! The values above were established using the WinAVR 2010 compiler.
//! The code runs faster when compiled with the compiler version of Atmel Studio 6.
//! In this case a timeout value of 26 leads to a timeout of 10 us which is still
//! greater than 8.6 us.
//! This value is decremented while waiting for the falling edge of a zero pulse.
#define ZERO_PULSE_TIME_OUT     (26)

/** @} */


/**
 * \brief Set SWI signal pin.
 *        Other functions will use this pin.
 *
 * \param[in] id  definition of GPIO pin to be used
 */
void swi_set_pin(uint8_t id);

/**
 * \brief Configure GPIO pin for SWI signal as output.
 */
void swi_enable(void);

/**
 * \brief Configure GPIO pin for SWI signal as input.
 */
void swi_disable(void);

/**
 * \brief Set signal pin Low or High.
 *
 * \param[in] is_high  0: Low, else: High.
 */
void swi_set_signal_pin(uint8_t is_high);

/**
 * \brief Send a Wake Token.
 */
void swi_send_wake_token(void);

/**
 * \brief Send a number of bytes.
 *
 * \param[in] count   number of bytes to send.
 * \param[in] buffer  pointer to buffer containing bytes to send
 */
void swi_send_bytes(uint8_t count, uint8_t *buffer);

/**
 * \brief Send one byte.
 *
 * \param[in] byte  byte to send
 */
void swi_send_byte(uint8_t byte);

/**
 * \brief Receive a number of bytes.
 *
 * \param[in]  count   number of bytes to receive
 * \param[out] buffer  pointer to receive buffer
 *
 * \return ATCA_STATUS
 */
ATCA_STATUS swi_receive_bytes(uint8_t count, uint8_t *buffer);

#endif /* SWI_BITBANG_AT88CK9000_H_ */