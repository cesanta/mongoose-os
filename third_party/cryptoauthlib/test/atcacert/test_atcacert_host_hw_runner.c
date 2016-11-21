/**
 * \file
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

#include "test/unity.h"
#include "test/unity_fixture.h"

#ifdef __GNUC__
// Unity macros trigger this warning
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif

TEST_GROUP_RUNNER(atcacert_host_hw)
{
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_cert_hw);
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_cert_hw_verify_failed);
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_cert_hw_short_cert);
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_cert_hw_bad_sig);
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_cert_hw_bad_params);

	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_gen_challenge_hw);
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_gen_challenge_hw_bad_params);

	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw);
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw_bad_challenge);
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw_bad_response);
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw_bad_public_key);
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw_malformed_public_key);
	RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw_bad_params);
}
