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

TEST_GROUP_RUNNER(atcacert_der_enc_ecdsa_sig_value)
{
	RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, no_padding);
	RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, r_padding);
	RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, s_padding);
	RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, rs_padding);
	RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, trim);
	RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, trim_all);
	RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, small_buf);
	RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, bad_params);
}

TEST_GROUP_RUNNER(atcacert_der_dec_ecdsa_sig_value)
{
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, no_padding);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, r_padding);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, s_padding);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, rs_padding);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, trim);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, trim_all);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_bs_tag);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_bs_length_low);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_bs_length_high);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_bs_extra_data);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_bs_spare_bits);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_seq_tag);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_seq_length_low);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_seq_length_high);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_seq_extra_data);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_rint_tag);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_rint_length_low);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_rint_length_high);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_sint_tag);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_sint_length_low);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_sint_length_high);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_rint_too_large);
	RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, bad_sint_too_large);
}