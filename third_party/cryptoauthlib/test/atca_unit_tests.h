/**
 * \file
 *
 * \brief  Unit tests for atcalib
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

#ifndef _ATCA_UTESTS_H
#define _ATCA_UTESTS_H

#include "unity_fixture.h"

extern ATCAIfaceCfg *gCfg;

int atca_sha204a_unit_tests(ATCADeviceType deviceType);
int atca_ecc108a_unit_tests(ATCADeviceType deviceType);
int atca_ecc508a_unit_tests(ATCADeviceType deviceType);
int atca_aes132a_unit_tests(ATCADeviceType deviceType);
int atca_unit_tests(ATCADeviceType deviceType);
int certdata_unit_tests(void);
int certio_unit_tests(void);
int atca_is_locked(uint8_t zone, uint8_t *lock_state);
void test_lock(void);
int atcau_get_addr(uint8_t zone, uint8_t slot, uint8_t block, uint8_t offset, uint16_t* addr);
int atcau_is_locked(uint8_t zone, uint8_t *lock_state);
void test_lock_zone(void);

void test_objectNew(void);
void test_objectDelete(void);

// basic command tests
void test_wake_sleep(void);
void test_wake_idle(void);
void test_crcerror(void);
void test_checkmac(void);
void test_counter(void);
void test_derivekey(void);
void test_ecdh(void);
void test_gendig(void);
void test_genkey(void);
void test_hmac(void);
void test_info(void);
void test_lock_config_zone(void);
void test_lock_data_zone(void);
void test_mac(void);
void test_nonce_passthrough(void);
void test_pause(void);
void test_privwrite(void);
void test_random(void);
void test_read(void);
void test_sha(void);
void test_sign(void);
void test_updateExtra(void);
void test_verify(void);
void test_write(void);
void test_devRev(void);

#endif
