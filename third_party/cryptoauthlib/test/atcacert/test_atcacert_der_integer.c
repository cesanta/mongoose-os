/**
 * \file
 * \brief cert DER format tests
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



#include "atcacert/atcacert_der.h"
#include "test/unity.h"
#include "test/unity_fixture.h"
#include <string.h>

TEST_GROUP(atcacert_der_enc_integer);

TEST_SETUP(atcacert_der_enc_integer)
{
}

TEST_TEAR_DOWN(atcacert_der_enc_integer)
{
}

static void atcacert_der_enc_integer_test(const uint8_t* int_data, size_t int_data_size, const uint8_t* der_int_ref, size_t der_int_ref_size, uint8_t is_signed)
{
	int ret = 0;
	uint8_t der_int[300];
	size_t der_int_size = sizeof(der_int);

	if (der_int_ref_size > sizeof(der_int))
		TEST_FAIL_MESSAGE("der_int isn't large enough to run this test.");

	ret = atcacert_der_enc_integer(int_data, int_data_size, is_signed, der_int, &der_int_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(der_int_ref_size, der_int_size);
	TEST_ASSERT_EQUAL_MEMORY(der_int_ref, der_int, der_int_ref_size);

	// Size only
	der_int_size = 0;
	ret = atcacert_der_enc_integer(int_data, int_data_size, is_signed, NULL, &der_int_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(der_int_ref_size, der_int_size);
}

TEST(atcacert_der_enc_integer, signed_min)
{
	uint8_t int_data[] = { 0x00 };
	uint8_t der_int_ref[] = { 0x02, 0x01, 0x00 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), FALSE);
}

TEST(atcacert_der_enc_integer, signed_1byte)
{
	uint8_t int_data[] = { 0x64 };
	uint8_t der_int_ref[] = { 0x02, 0x01, 0x64 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), FALSE);
}

TEST(atcacert_der_enc_integer, signed_multi_byte)
{
	uint8_t int_data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
	uint8_t der_int_ref[] = { 0x02, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), FALSE);
}

TEST(atcacert_der_enc_integer, signed_large)
{
	uint8_t int_data[256];
	uint8_t der_int_ref[1 + 3 + sizeof(int_data)];
	size_t i;

	for (i = 0; i < sizeof(int_data); i++)
		int_data[i] = (uint8_t)(i + 1);

	der_int_ref[0] = 0x02;
	der_int_ref[1] = 0x82;
	der_int_ref[2] = (uint8_t)((sizeof(int_data) >> 8) & 0xFF);
	der_int_ref[3] = (uint8_t)((sizeof(int_data) >> 0) & 0xFF);
	memcpy(&der_int_ref[4], int_data, sizeof(int_data));

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), FALSE);
}

TEST(atcacert_der_enc_integer, signed_trim_1_pos)
{
	uint8_t int_data[] = { 0x00, 0x02, 0x03, 0x04, 0x05 };
	uint8_t der_int_ref[] = { 0x02, 0x04, 0x02, 0x03, 0x04, 0x05 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), FALSE);
}

TEST(atcacert_der_enc_integer, signed_trim_multi_pos)
{
	uint8_t int_data[] = { 0x00, 0x00, 0x00, 0x04, 0x05 };
	uint8_t der_int_ref[] = { 0x02, 0x02, 0x04, 0x05 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), FALSE);
}

TEST(atcacert_der_enc_integer, signed_trim_all_pos)
{
	uint8_t int_data[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t der_int_ref[] = { 0x02, 0x01, 0x00 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), FALSE);
}

TEST(atcacert_der_enc_integer, signed_trim_1_neg)
{
	uint8_t int_data[] = { 0xFF, 0xFE, 0xFD, 0xFC, 0xFB };
	uint8_t der_int_ref[] = { 0x02, 0x04, 0xFE, 0xFD, 0xFC, 0xFB };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), FALSE);
}

TEST(atcacert_der_enc_integer, signed_trim_multi_neg)
{
	uint8_t int_data[] = { 0xFF, 0xFF, 0xFF, 0xFC, 0xFB };
	uint8_t der_int_ref[] = { 0x02, 0x02, 0xFC, 0xFB };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), FALSE);
}

TEST(atcacert_der_enc_integer, signed_trim_all_neg)
{
	uint8_t int_data[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	uint8_t der_int_ref[] = { 0x02, 0x01, 0xFF };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), FALSE);
}

TEST(atcacert_der_enc_integer, unsigned_min)
{
	uint8_t int_data[] = { 0x00 };
	uint8_t der_int_ref[] = { 0x02, 0x01, 0x00 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), TRUE);
}

TEST(atcacert_der_enc_integer, unsigned_min_pad)
{
	uint8_t int_data[] = { 0x80 };
	uint8_t der_int_ref[] = { 0x02, 0x02, 0x00, 0x80 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), TRUE);
}

TEST(atcacert_der_enc_integer, unsigned_multi_byte)
{
	uint8_t int_data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
	uint8_t der_int_ref[] = { 0x02, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), TRUE);
}

TEST(atcacert_der_enc_integer, unsigned_multi_byte_pad)
{
	uint8_t int_data[] = { 0x81, 0x02, 0x03, 0x04, 0x05 };
	uint8_t der_int_ref[] = { 0x02, 0x06, 0x00, 0x81, 0x02, 0x03, 0x04, 0x05 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), TRUE);
}

TEST(atcacert_der_enc_integer, unsigned_large)
{
	uint8_t int_data[256];
	uint8_t der_int_ref[1 + 3 + sizeof(int_data)];
	size_t i;

	for (i = 0; i < sizeof(int_data); i++)
		int_data[i] = (uint8_t)(i + 1);

	der_int_ref[0] = 0x02;
	der_int_ref[1] = 0x82;
	der_int_ref[2] = (uint8_t)((sizeof(int_data) >> 8) & 0xFF);
	der_int_ref[3] = (uint8_t)((sizeof(int_data) >> 0) & 0xFF);
	memcpy(&der_int_ref[4], int_data, sizeof(int_data));

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), TRUE);
}

TEST(atcacert_der_enc_integer, unsigned_large_pad)
{
	uint8_t int_data[256];
	uint8_t der_int_ref[1 + 3 + 1 + sizeof(int_data)];
	size_t i;

	for (i = 0; i < sizeof(int_data); i++)
		int_data[i] = (uint8_t)(i + 1);
	int_data[0] = 0x81;

	der_int_ref[0] = 0x02;
	der_int_ref[1] = 0x82;
	der_int_ref[2] = (uint8_t)(((sizeof(int_data) + 1) >> 8) & 0xFF);
	der_int_ref[3] = (uint8_t)(((sizeof(int_data) + 1) >> 0) & 0xFF);
	der_int_ref[4] = 0x00;
	memcpy(&der_int_ref[5], int_data, sizeof(int_data));

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), TRUE);
}

TEST(atcacert_der_enc_integer, unsigned_trim_1_pos)
{
	uint8_t int_data[] = { 0x00, 0x02, 0x03, 0x04, 0x05 };
	uint8_t der_int_ref[] = { 0x02, 0x04, 0x02, 0x03, 0x04, 0x05 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), TRUE);
}

TEST(atcacert_der_enc_integer, unsigned_trim_multi_pos)
{
	uint8_t int_data[] = { 0x00, 0x00, 0x00, 0xF4, 0x05 };
	uint8_t der_int_ref[] = { 0x02, 0x03, 0x00, 0xF4, 0x05 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), TRUE);
}

TEST(atcacert_der_enc_integer, unsigned_trim_all_pos)
{
	uint8_t int_data[] = { 0x00, 0x00, 0x00, 0x00, 0x80 };
	uint8_t der_int_ref[] = { 0x02, 0x02, 0x00, 0x80 };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), TRUE);
}

TEST(atcacert_der_enc_integer, unsigned_trim_neg_pad)
{
	uint8_t int_data[] = { 0xFF, 0xFE, 0xFD, 0xFC, 0xFB };
	uint8_t der_int_ref[] = { 0x02, 0x06, 0x00, 0xFF, 0xFE, 0xFD, 0xFC, 0xFB };

	atcacert_der_enc_integer_test(int_data, sizeof(int_data), der_int_ref, sizeof(der_int_ref), TRUE);
}

TEST(atcacert_der_enc_integer, small_buf)
{
	int ret = 0;
	uint8_t int_data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
	uint8_t der_int[sizeof(int_data) + 1];
	size_t der_int_size = sizeof(der_int);

	ret = atcacert_der_enc_integer(int_data, sizeof(int_data), FALSE, der_int, &der_int_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BUFFER_TOO_SMALL, ret);
	TEST_ASSERT_EQUAL(sizeof(int_data) + 2, der_int_size);
}

TEST(atcacert_der_enc_integer, bad_params)
{
	int ret = 0;
	uint8_t int_data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
	uint8_t der_int[sizeof(int_data) + 2];
	size_t der_int_size = sizeof(der_int);

	ret = atcacert_der_enc_integer(NULL, sizeof(int_data), FALSE, der_int, &der_int_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_enc_integer(int_data, sizeof(int_data), FALSE, der_int, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_enc_integer(NULL, sizeof(int_data), FALSE, der_int, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}

TEST_GROUP(atcacert_der_dec_integer);

TEST_SETUP(atcacert_der_dec_integer)
{
}

TEST_TEAR_DOWN(atcacert_der_dec_integer)
{
}

struct good_s {
	size_t int_data_size;
	uint8_t int_data[3];
	size_t der_integer_size;
	uint8_t der_integer[5];
};

TEST(atcacert_der_dec_integer, good)
{
	int ret = 0;
	size_t i;

	const struct good_s tests[] = {
		{ 1, { 0x00, 0x00, 0x00 }, 3, { 0x02, 0x01, 0x00, 0x00, 0x00 } },
		{ 1, { 0x01, 0x00, 0x00 }, 3, { 0x02, 0x01, 0x01, 0x00, 0x00 } },
		{ 1, { 0xFF, 0x00, 0x00 }, 3, { 0x02, 0x01, 0xFF, 0x00, 0x00 } },
		{ 2, { 0x01, 0x02, 0x00 }, 4, { 0x02, 0x02, 0x01, 0x02, 0x00 } },
		{ 2, { 0xFF, 0xFF, 0x00 }, 4, { 0x02, 0x02, 0xFF, 0xFF, 0x00 } },
		{ 3, { 0x01, 0x02, 0x03 }, 5, { 0x02, 0x03, 0x01, 0x02, 0x03 } },
		{ 3, { 0xFF, 0xFF, 0xFF }, 5, { 0x02, 0x03, 0xFF, 0xFF, 0xFF } }
	};

	for (i = 0; i < sizeof(tests) / sizeof(struct good_s); i++) {
		size_t der_integer_size = sizeof(tests[i].der_integer);
		uint8_t int_data[3];
		size_t int_data_size = sizeof(int_data);

		ret = atcacert_der_dec_integer(tests[i].der_integer, &der_integer_size, int_data, &int_data_size);
		TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
		TEST_ASSERT_EQUAL(tests[i].der_integer_size, der_integer_size);
		TEST_ASSERT_EQUAL(tests[i].int_data_size, int_data_size);
		TEST_ASSERT_EQUAL_MEMORY(tests[i].int_data, int_data, int_data_size);

		// DER size only
		der_integer_size = sizeof(tests[i].der_integer);
		ret = atcacert_der_dec_integer(tests[i].der_integer, &der_integer_size, NULL, NULL);
		TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
		TEST_ASSERT_EQUAL(tests[i].der_integer_size, der_integer_size);

		// int_data_size only
		der_integer_size = sizeof(tests[i].der_integer);
		ret = atcacert_der_dec_integer(tests[i].der_integer, &der_integer_size, NULL, &int_data_size);
		TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
		TEST_ASSERT_EQUAL(tests[i].der_integer_size, der_integer_size);
		TEST_ASSERT_EQUAL(tests[i].int_data_size, int_data_size);
	}
}

TEST(atcacert_der_dec_integer, good_large)
{
	int ret = 0;
	uint8_t int_data_ref[256];
	uint8_t der_integer[1 + 3 + sizeof(int_data_ref)];
	size_t der_integer_size = sizeof(der_integer);
	uint8_t int_data[sizeof(int_data_ref)];
	size_t int_data_size = sizeof(int_data);
	size_t i;

	for (i = 0; i < sizeof(int_data_ref); i++)
		int_data_ref[i] = (uint8_t)(i + 1);

	der_integer[0] = 0x02;
	der_integer[1] = 0x82;
	der_integer[2] = (uint8_t)((sizeof(int_data_ref) >> 8) & 0xFF);
	der_integer[3] = (uint8_t)((sizeof(int_data_ref) >> 0) & 0xFF);
	memcpy(&der_integer[4], int_data_ref, sizeof(int_data_ref));

	ret = atcacert_der_dec_integer(der_integer, &der_integer_size, int_data, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_integer), der_integer_size);
	TEST_ASSERT_EQUAL(sizeof(int_data_ref), int_data_size);
	TEST_ASSERT_EQUAL_MEMORY(int_data_ref, int_data, int_data_size);

	// DER size only
	der_integer_size = sizeof(der_integer);
	ret = atcacert_der_dec_integer(der_integer, &der_integer_size, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_integer), der_integer_size);

	// int_data_size only
	der_integer_size = sizeof(der_integer);
	ret = atcacert_der_dec_integer(der_integer, &der_integer_size, NULL, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(sizeof(der_integer), der_integer_size);
	TEST_ASSERT_EQUAL(sizeof(int_data_ref), int_data_size);
}

TEST(atcacert_der_dec_integer, zero_size)
{
	int ret = 0;
	uint8_t der_integer[] = { 0x00 };
	size_t der_integer_size = 0;
	uint8_t int_data[3];
	size_t int_data_size = sizeof(int_data);

	ret = atcacert_der_dec_integer(der_integer, &der_integer_size, int_data, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_der_dec_integer, not_enough_data)
{
	int ret = 0;
	uint8_t der_integer[] = { 0x02, 0x02, 0x01, 0x02, 0x03 };
	size_t der_integer_size = 0;
	uint8_t int_data[3];
	size_t int_data_size = sizeof(int_data);

	der_integer_size = 1;
	int_data_size = sizeof(int_data);
	ret = atcacert_der_dec_integer(der_integer, &der_integer_size, int_data, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	der_integer_size = 2;
	int_data_size = sizeof(int_data);
	ret = atcacert_der_dec_integer(der_integer, &der_integer_size, int_data, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	der_integer_size = 3;
	int_data_size = sizeof(int_data);
	ret = atcacert_der_dec_integer(der_integer, &der_integer_size, int_data, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	der_integer[1] = 0x81;
	der_integer[2] = 0x02;
	der_integer_size = 4;
	int_data_size = sizeof(int_data);
	ret = atcacert_der_dec_integer(der_integer, &der_integer_size, int_data, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_der_dec_integer, small_buf)
{
	int ret = 0;
	uint8_t der_integer[] = { 0x02, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05 };
	size_t der_integer_size = sizeof(der_integer);
	uint8_t int_data[3];
	size_t int_data_size = sizeof(int_data);

	ret = atcacert_der_dec_integer(der_integer, &der_integer_size, int_data, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BUFFER_TOO_SMALL, ret);
	TEST_ASSERT_EQUAL(5, int_data_size);
}

TEST(atcacert_der_dec_integer, bad_params)
{
	int ret = 0;
	uint8_t der_integer[] = { 0x02, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05 };
	size_t der_integer_size = sizeof(der_integer);
	uint8_t int_data[5];
	size_t int_data_size = sizeof(int_data);

	ret = atcacert_der_dec_integer(NULL, &der_integer_size, int_data, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(der_integer, NULL, int_data, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(NULL, NULL, int_data, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(NULL, &der_integer_size, NULL, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(der_integer, NULL, NULL, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(NULL, NULL, NULL, &int_data_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(der_integer, &der_integer_size, int_data, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(NULL, &der_integer_size, int_data, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(der_integer, NULL, int_data, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(NULL, NULL, int_data, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(NULL, &der_integer_size, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(der_integer, NULL, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_der_dec_integer(NULL, NULL, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}