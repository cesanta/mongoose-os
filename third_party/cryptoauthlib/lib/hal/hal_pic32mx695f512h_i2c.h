/**
 * \file
 * \brief ATCA Hardware abstraction layer for PIC32MX695F512H I2C over xxx drivers.
 *
 * This code is structured in two parts.  Part 1 is the connection of the ATCA HAL API to the physical I2C
 * implementation. Part 2 is the xxx I2C primitives to set up the interface.
 *
 * Prerequisite: 
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

#ifndef HAL_PIC32MX695F512H_I2C_ASF_H_
#define HAL_PIC32MX695F512H_I2C_ASF_H_


/**
 * \defgroup hal_ Hardware abstraction layer (hal_)
 *
 * \brief
 * These methods define the hardware abstraction layer for communicating with a CryptoAuth device
 * using I2C driver of ASF.
 *
@{ */

// Clock Constants
#define	GetSystemClock()        (80000000ul)
#define	GetPeripheralClock()    (GetSystemClock()/(1 << OSCCONbits.PBDIV))
#define	GetInstructionClock()   (GetSystemClock())

#define MAX_I2C_BUSES           4   // PIC32MX695F512H has 4 TWI

/**
 * \brief this is the hal_data for ATCA HAL
 */
typedef struct atcaI2Cmaster {
	I2C_MODULE id;
	int ref_ct;
	// for conveniences during interface release phase
	int bus_index;
} ATCAI2CMaster_t;

void i2c_write(I2C_MODULE i2c_id, uint8_t address, uint8_t *data, int len);
void i2c_read(I2C_MODULE i2c_id, uint8_t address, uint8_t *data, uint16_t len);

void change_i2c_speed(ATCAIface iface, uint32_t speed);

/** @} */

#endif  /* HAL_PIC32MX695F512H_I2C_ASF_H_ */