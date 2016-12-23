/**
 * \file
 * \brief Unity tests for the cryptoauthlib Basic API
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

#ifndef ATCA_BASIC_TESTS_H_
#define ATCA_BASIC_TESTS_H_

#include "cryptoauthlib.h"
#include "unity.h"

void atca_sha204a_basic_tests(ATCADeviceType deviceType);
void atca_ecc108a_basic_tests(ATCADeviceType deviceType);
void atca_ecc508a_basic_tests(ATCADeviceType deviceType);
void atca_aes132a_basic_tests(ATCADeviceType deviceType);
void atca_basic_tests(ATCADeviceType deviceType);

void test_basic_version(void);
void test_basic_init(void);
void test_basic_release(void);
void test_basic_doubleinit(void);
void test_basic_info(void);
void test_basic_random(void);
void test_basic_challenge(void);
void test_basic_write_zone(void);
void test_basic_read_zone(void);
void test_write_boundary_conditions(void);
void test_write_upper_slots(void);
void test_write_invalid_block(void);
void test_write_invalid_block_len(void);
void test_write_bytes_slot(void);
void test_write_bytes_zone_config(void);
void test_write_bytes_zone_slot8(void);
void test_basic_write_enc(void);
void test_basic_write_ecc_config_zone(void);
void test_basic_read_config_zone(void);
void test_basic_write_config_zone(void);
void test_basic_read_ecc_config_zone(void);
void test_basic_write_sha_config_zone(void);
void test_basic_read_sha_config_zone(void);
void test_basic_write_otp_zone(void);
void test_basic_read_otp_zone(void);
void test_basic_write_slot4_key(void);
void test_basic_write_data_zone(void);
void test_basic_read_data_zone(void);
void test_basic_lock_config_zone(void);
void test_basic_lock_data_zone(void);
void test_basic_lock_data_slot(void);
void test_basic_genkey(void);
void test_basic_sign(void);
void test_basic_sign_internal(void);
void test_read_sig(void);
void test_basic_priv_write_unencrypted(void);
void test_basic_priv_write_encrypted(void);
void test_basic_verify_extern(void);
void test_basic_verify_stored(void);
void test_basic_verify_validate(void);
void test_basic_verify_invalidate(void);
void test_basic_ecdh(void);
void test_basic_gendig(void);
void test_basic_mac(void);
void test_basic_checkmac(void);
void test_basic_hmac(void);
void test_basic_derivekey(void);
void test_basic_derivekey_mac(void);
void test_basic_sha(void);
void test_basic_sha_long(void);
void test_basic_sha_short(void);
void test_basic_hw_sha2_256_nist1(void);
void test_basic_hw_sha2_256_nist2(void);
void test_basic_hw_sha2_256_nist_short(void);
void test_basic_hw_sha2_256_nist_long(void);
void test_basic_hw_sha2_256_nist_monte(void);

// Helper tests
void atca_helper_tests(void);
void test_base64encode_decode(void);

// test the extra function used with provisioning FW
// use the default customer configuration same as the provisioning config for the configuration zone
void test_create_key(void);
void test_get_pubkey(void);

#endif /* ATCA_BASIC_TESTS_H_ */