/**
 * \file
 * \brief cert date tests
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



#include "atcacert/atcacert_date.h"
#include "test/unity.h"
#include "test/unity_fixture.h"
#include <string.h>

#undef min
#undef max

static void set_tm(atcacert_tm_utc_t* ts, int year, int month, int day, int hour, int min, int sec)
{
	size_t size = sizeof(*ts);

	memset(ts, 0, size);

	ts->tm_year = year - 1900;
	ts->tm_mon = month - 1;
	ts->tm_mday = day;
	ts->tm_hour = hour;
	ts->tm_min = min;
	ts->tm_sec = sec;
}

TEST_GROUP(atcacert_date_enc_iso8601_sep);

TEST_SETUP(atcacert_date_enc_iso8601_sep)
{
}

TEST_TEAR_DOWN(atcacert_date_enc_iso8601_sep)
{
}

TEST(atcacert_date_enc_iso8601_sep, good)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = "2013-11-10T09:08:07Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_iso8601_sep, min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = "0000-01-01T00:00:00Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 0, 1, 1, 0, 0, 0);

	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_iso8601_sep, max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = "9999-12-31T23:59:59Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 9999, 12, 31, 23, 59, 59);

	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_iso8601_sep, bad_year)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, -1, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 10000, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, bad_month)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 0, 10, 9, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 13, 10, 9, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, bad_day)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 0, 9, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 32, 9, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, bad_hour)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, -1, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 10, 24, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, bad_min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, -1, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 10, 9, 60, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, bad_sec)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, -1);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 60);
	ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, bad_params)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(NULL, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(&ts, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_iso8601_sep(NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}




TEST_GROUP(atcacert_date_enc_rfc5280_utc);

TEST_SETUP(atcacert_date_enc_rfc5280_utc)
{
}

TEST_TEAR_DOWN(atcacert_date_enc_rfc5280_utc)
{
}

TEST(atcacert_date_enc_rfc5280_utc, good)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = "131110090807Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_utc, min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = "500101000000Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 1950, 1, 1, 0, 0, 0);

	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_utc, max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = "491231235959Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2049, 12, 31, 23, 59, 59);

	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_utc, y2k)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	char ts_str_ref[sizeof(ts_str) + 1];
	atcacert_tm_utc_t ts;

	memcpy(ts_str_ref, "991231235959Z", sizeof(ts_str_ref));
	set_tm(&ts, 1999, 12, 31, 23, 59, 59);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));

	memcpy(ts_str_ref, "000101000000Z", sizeof(ts_str_ref));
	set_tm(&ts, 2000, 1, 1, 0, 0, 0);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_utc, bad_year)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 1949, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2050, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, bad_month)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 0, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 13, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, bad_day)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 0, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 32, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, bad_hour)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, -1, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 10, 24, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, bad_min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, -1, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 10, 9, 60, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, bad_sec)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, -1);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 60);
	ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, bad_params)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(NULL, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(&ts, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_utc(NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}



TEST_GROUP(atcacert_date_enc_posix_uint32_be);

TEST_SETUP(atcacert_date_enc_posix_uint32_be)
{
}

TEST_TEAR_DOWN(atcacert_date_enc_posix_uint32_be)
{
}

TEST(atcacert_date_enc_posix_uint32_be, good)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = { 0x52, 0x7F, 0x4C, 0xF7 };
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_be, min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = { 0x00, 0x00, 0x00, 0x00 };
	atcacert_tm_utc_t ts;

	set_tm(&ts, 1970, 1, 1, 0, 0, 0);

	ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_be, large)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = { 0xFE, 0xFD, 0xFC, 0xFB };
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2105, 7, 26, 13, 30, 35);

	ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_be, max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
	atcacert_tm_utc_t ts;
	const char ts_str_ref[sizeof(ts_str) + 1] = { 0xFF, 0xFF, 0xFF, 0xFE };

	set_tm(&ts, 2106, 2, 7, 6, 28, 14);

	ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_be, bad_low)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 1969, 12, 31, 23, 59, 59);
	ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_posix_uint32_be, bad_high)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2106, 2, 7, 6, 28, 15);
	ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_posix_uint32_be, bad_params)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_posix_uint32_be(NULL, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_posix_uint32_be(&ts, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_posix_uint32_be(NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}



TEST_GROUP(atcacert_date_enc_posix_uint32_le);

TEST_SETUP(atcacert_date_enc_posix_uint32_le)
{
}

TEST_TEAR_DOWN(atcacert_date_enc_posix_uint32_le)
{
}

TEST(atcacert_date_enc_posix_uint32_le, good)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = { 0xF7, 0x4C, 0x7F, 0x52  };
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc_posix_uint32_le(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_le, min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = { 0x00, 0x00, 0x00, 0x00 };
	atcacert_tm_utc_t ts;

	set_tm(&ts, 1970, 1, 1, 0, 0, 0);

	ret = atcacert_date_enc_posix_uint32_le(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_le, large)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = { 0xFB, 0xFC, 0xFD, 0xFE };
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2105, 7, 26, 13, 30, 35);

	ret = atcacert_date_enc_posix_uint32_le(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_le, max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE];
	atcacert_tm_utc_t ts;
	const char ts_str_ref[sizeof(ts_str) + 1] = { 0xFE, 0xFF, 0xFF, 0xFF };

	set_tm(&ts, 2106, 2, 7, 6, 28, 14);

	ret = atcacert_date_enc_posix_uint32_le(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_le, bad_low)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 1969, 12, 31, 23, 59, 59);
	ret = atcacert_date_enc_posix_uint32_le(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_posix_uint32_le, bad_high)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2106, 2, 7, 6, 28, 15);
	ret = atcacert_date_enc_posix_uint32_le(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_posix_uint32_le, bad_params)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_posix_uint32_le(NULL, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_posix_uint32_le(&ts, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_posix_uint32_le(NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}



TEST_GROUP(atcacert_date_enc_rfc5280_gen);

TEST_SETUP(atcacert_date_enc_rfc5280_gen)
{
}

TEST_TEAR_DOWN(atcacert_date_enc_rfc5280_gen)
{
}

TEST(atcacert_date_enc_rfc5280_gen, good)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = "20131110090807Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_gen, min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = "00000101000000Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 0, 1, 1, 0, 0, 0);

	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_gen, max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
	const char ts_str_ref[sizeof(ts_str) + 1] = "99991231235959Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 9999, 12, 31, 23, 59, 59);

	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_gen, bad_year)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, -1, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 10000, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, bad_month)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 0, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 13, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, bad_day)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 0, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 32, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, bad_hour)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, -1, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 10, 24, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, bad_min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, -1, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 10, 9, 60, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, bad_sec)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, -1);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 60);
	ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, bad_params)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(NULL, ts_str);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(&ts, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);
	ret = atcacert_date_enc_rfc5280_gen(NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}




TEST_GROUP(atcacert_date_enc_compcert);

TEST_SETUP(atcacert_date_enc_compcert)
{
}

TEST_TEAR_DOWN(atcacert_date_enc_compcert)
{
}

TEST(atcacert_date_enc_compcert, good)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;
	uint8_t enc_dates[3];
	uint8_t enc_dates_ref[sizeof(enc_dates)] = { 0xA9, 0x9D, 0x5C };
	uint8_t expire_years = 28;

	set_tm(&issue_date, 2021, 3, 7, 10, 0, 0);

	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(enc_dates_ref, enc_dates, sizeof(enc_dates));
}

TEST(atcacert_date_enc_compcert, min)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;
	uint8_t enc_dates[3];
	uint8_t enc_dates_ref[sizeof(enc_dates)] = { 0x00, 0x84, 0x00 };
	uint8_t expire_years = 0;

	set_tm(&issue_date, 2000, 1, 1, 00, 00, 00);

	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(enc_dates_ref, enc_dates, sizeof(enc_dates));
}

TEST(atcacert_date_enc_compcert, max)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;
	uint8_t enc_dates[3];
	uint8_t enc_dates_ref[sizeof(enc_dates)] = { 0xFE, 0x7E, 0xFF };
	uint8_t expire_years = 31;

	set_tm(&issue_date, 2031, 12, 31, 23, 00, 00);

	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(enc_dates_ref, enc_dates, sizeof(enc_dates));
}

TEST(atcacert_date_enc_compcert, bad_year)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;
	uint8_t enc_dates[3];
	uint8_t expire_years = 0;

	expire_years = 28;
	set_tm(&issue_date, 1900, 3, 7, 10, 0, 0);
	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	expire_years = 28;
	set_tm(&issue_date, 2032, 3, 7, 10, 0, 0);
	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_compcert, bad_month)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;
	uint8_t enc_dates[3];
	uint8_t expire_years = 0;

	expire_years = 28;
	set_tm(&issue_date, 2021, 0, 7, 10, 0, 0);
	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	expire_years = 28;
	set_tm(&issue_date, 2021, 13, 7, 10, 0, 0);
	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_compcert, bad_day)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;
	uint8_t enc_dates[3];
	uint8_t expire_years = 0;

	expire_years = 28;
	set_tm(&issue_date, 2021, 3, 0, 10, 0, 0);
	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	expire_years = 28;
	set_tm(&issue_date, 2021, 3, 32, 10, 0, 0);
	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_compcert, bad_hour)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;
	uint8_t enc_dates[3];
	uint8_t expire_years = 0;

	expire_years = 28;
	set_tm(&issue_date, 2021, 3, 7, -1, 0, 0);
	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

	expire_years = 28;
	set_tm(&issue_date, 2021, 3, 7, 24, 0, 0);
	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_compcert, bad_expire)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;
	uint8_t enc_dates[3];
	uint8_t expire_years = 0;

	expire_years = 32;
	set_tm(&issue_date, 2021, 3, 7, 10, 0, 0);
	ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_compcert, bad_params)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;
	uint8_t enc_dates[3];
	uint8_t expire_years = 0;

	expire_years = 28;
	set_tm(&issue_date, 2021, 3, 7, 10, 0, 0);
	ret = atcacert_date_enc_compcert(NULL, expire_years, enc_dates);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	expire_years = 28;
	set_tm(&issue_date, 2021, 3, 7, 10, 0, 0);
	ret = atcacert_date_enc_compcert(&issue_date, expire_years, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	expire_years = 28;
	set_tm(&issue_date, 2021, 3, 7, 10, 0, 0);
	ret = atcacert_date_enc_compcert(NULL, expire_years, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}




TEST_GROUP(atcacert_date_enc);

TEST_SETUP(atcacert_date_enc)
{
}

TEST_TEAR_DOWN(atcacert_date_enc)
{
}

TEST(atcacert_date_enc, iso8601_sep)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1];
	size_t ts_str_size = sizeof(ts_str);
	const char ts_str_ref[sizeof(ts_str)] = "2013-11-10T09:08:07Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc(DATEFMT_ISO8601_SEP, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(DATEFMT_ISO8601_SEP_SIZE, ts_str_size);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, ts_str_size);

	// Size only
	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_ISO8601_SEP, &ts, NULL, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(DATEFMT_ISO8601_SEP_SIZE, ts_str_size);
}

TEST(atcacert_date_enc, rfc5280_utc)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1];
	size_t ts_str_size = sizeof(ts_str);
	const char ts_str_ref[sizeof(ts_str)] = "131110090807Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc(DATEFMT_RFC5280_UTC, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(DATEFMT_RFC5280_UTC_SIZE, ts_str_size);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, ts_str_size);

	// Size only
	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_RFC5280_UTC, &ts, NULL, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(DATEFMT_RFC5280_UTC_SIZE, ts_str_size);
}

TEST(atcacert_date_enc, posix_uint32_be)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE + 1];
	size_t ts_str_size = sizeof(ts_str);
	const char ts_str_ref[sizeof(ts_str) - 1] = { 0x52, 0x7F, 0x4C, 0xF7 };
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc(DATEFMT_POSIX_UINT32_BE, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(DATEFMT_POSIX_UINT32_BE_SIZE, ts_str_size);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, ts_str_size);

	// Size only
	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_POSIX_UINT32_BE, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(DATEFMT_POSIX_UINT32_BE_SIZE, ts_str_size);
}

TEST(atcacert_date_enc, posix_uint32_le)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE + 1];
	size_t ts_str_size = sizeof(ts_str);
	const char ts_str_ref[sizeof(ts_str) - 1] = { 0xF7, 0x4C, 0x7F, 0x52 };
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc(DATEFMT_POSIX_UINT32_LE, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(DATEFMT_POSIX_UINT32_BE_SIZE, ts_str_size);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, ts_str_size);

	// Size only
	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_POSIX_UINT32_LE, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(DATEFMT_POSIX_UINT32_BE_SIZE, ts_str_size);
}

TEST(atcacert_date_enc, rfc5280_gen)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
	size_t ts_str_size = sizeof(ts_str);
	const char ts_str_ref[sizeof(ts_str)] = "20131110090807Z";
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc(DATEFMT_RFC5280_GEN, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(DATEFMT_RFC5280_GEN_SIZE, ts_str_size);
	TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, ts_str_size);

	// Size only
	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_RFC5280_GEN, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL(DATEFMT_RFC5280_GEN_SIZE, ts_str_size);
}

TEST(atcacert_date_enc, small_buf)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE - 1];
	size_t ts_str_size = sizeof(ts_str);
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc(DATEFMT_RFC5280_UTC, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BUFFER_TOO_SMALL, ret);
	TEST_ASSERT_EQUAL(DATEFMT_RFC5280_UTC_SIZE, ts_str_size);
}

TEST(atcacert_date_enc, bad_format)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
	size_t ts_str_size = sizeof(ts_str);
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_enc(-1, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_enc(DATEFMT_RFC5280_GEN + 1, &ts, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}

TEST(atcacert_date_enc, bad_params)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
	size_t ts_str_size = sizeof(ts_str);
	atcacert_tm_utc_t ts;

	set_tm(&ts, 2013, 11, 10, 9, 8, 7);

	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_RFC5280_GEN, NULL, ts_str, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_RFC5280_GEN, NULL, NULL, &ts_str_size);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_RFC5280_GEN, &ts, ts_str, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_RFC5280_GEN, NULL, ts_str, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_RFC5280_GEN, &ts, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_enc(DATEFMT_RFC5280_GEN, NULL, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}




TEST_GROUP(atcacert_date_dec_iso8601_sep);

TEST_SETUP(atcacert_date_dec_iso8601_sep)
{
}

TEST_TEAR_DOWN(atcacert_date_dec_iso8601_sep)
{
}

TEST(atcacert_date_dec_iso8601_sep, good)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1] = "2014-12-11T10:09:08Z";
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2014, 12, 11, 10, 9, 8);

	ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_iso8601_sep, min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1] = "0000-01-01T00:00:00Z";
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 0, 1, 1, 0, 0, 0);

	ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_iso8601_sep, max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1] = "9999-12-31T23:59:59Z";
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 9999, 12, 31, 23, 59, 59);

	ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_iso8601_sep, bad_int)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1];
	atcacert_tm_utc_t ts;

	memcpy(ts_str, "A014-12-11T10:09:08Z", sizeof(ts_str));
	ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "2014-A2-11T10:09:08Z", sizeof(ts_str));
	ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "2014-12-A1T10:09:08Z", sizeof(ts_str));
	ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "2014-12-11TA0:09:08Z", sizeof(ts_str));
	ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "2014-12-11T10:A9:08Z", sizeof(ts_str));
	ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "2014-12-11T10:09:A8Z", sizeof(ts_str));
	ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_date_dec_iso8601_sep, bad_params)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1];
	atcacert_tm_utc_t ts;

	memcpy(ts_str, "2014-12-11T10:09:08Z", sizeof(ts_str));
	ret = atcacert_date_dec_iso8601_sep(NULL, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	memcpy(ts_str, "2014-12-11T10:09:08Z", sizeof(ts_str));
	ret = atcacert_date_dec_iso8601_sep(ts_str, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	memcpy(ts_str, "2014-12-11T10:09:08Z", sizeof(ts_str));
	ret = atcacert_date_dec_iso8601_sep(NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}




TEST_GROUP(atcacert_date_dec_rfc5280_utc);

TEST_SETUP(atcacert_date_dec_rfc5280_utc)
{
}

TEST_TEAR_DOWN(atcacert_date_dec_rfc5280_utc)
{
}

TEST(atcacert_date_dec_rfc5280_utc, good)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1] = "141211100908Z";
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2014, 12, 11, 10, 9, 8);

	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_utc, min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1] = "500101000000Z";
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 1950, 1, 1, 0, 0, 0);

	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_utc, max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1] = "491231235959Z";
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2049, 12, 31, 23, 59, 59);

	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_utc, y2k)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1];
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	memcpy(ts_str, "991231235959Z", sizeof(ts_str));
	set_tm(&ts_ref, 1999, 12, 31, 23, 59, 59);
	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));

	memcpy(ts_str, "000101000000Z", sizeof(ts_str));
	set_tm(&ts_ref, 2000, 1, 1, 0, 0, 0);
	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_utc, bad_int)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1];
	atcacert_tm_utc_t ts;

	memcpy(ts_str, "A41211100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "14A211100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "1412A1100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "141211A00908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "14121110A908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "1412111009A8Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_date_dec_rfc5280_utc, bad_params)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1];
	atcacert_tm_utc_t ts;

	memset(ts_str, 0, sizeof(ts_str));

	memcpy(ts_str, "141211100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_utc(NULL, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	memcpy(ts_str, "141211100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_utc(ts_str, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	memcpy(ts_str, "141211100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_utc(NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}



TEST_GROUP(atcacert_date_dec_posix_uint32_be);

TEST_SETUP(atcacert_date_dec_posix_uint32_be)
{
}

TEST_TEAR_DOWN(atcacert_date_dec_posix_uint32_be)
{
}

TEST(atcacert_date_dec_posix_uint32_be, good)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0x52, 0x7F, 0x4C, 0xF7 };
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_dec_posix_uint32_be(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_be, min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0x00, 0x00, 0x00, 0x00 };
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 1970, 1, 1, 0, 0, 0);

	ret = atcacert_date_dec_posix_uint32_be(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_be, int32_max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0x7F, 0xFF, 0xFF, 0xFF };
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2038, 1, 19, 3, 14, 7);

	ret = atcacert_date_dec_posix_uint32_be(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_be, large)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0xFE, 0xFD, 0xFC, 0xFB };
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2105, 7, 26, 13, 30, 35);

	ret = atcacert_date_dec_posix_uint32_be(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_be, max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0xFF, 0xFF, 0xFF, 0xFE };
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2106, 2, 7, 6, 28, 14);

	ret = atcacert_date_dec_posix_uint32_be(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_be, bad_params)
{
	int ret = 0;
	uint8_t ts_str_good[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0x52, 0x7F, 0x4C, 0xF7 };
	uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
	atcacert_tm_utc_t ts;

	memcpy(ts_str, ts_str_good, sizeof(ts_str));
	ret = atcacert_date_dec_posix_uint32_be(NULL, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	memcpy(ts_str, ts_str_good, sizeof(ts_str));
	ret = atcacert_date_dec_posix_uint32_be(ts_str, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	memcpy(ts_str, ts_str_good, sizeof(ts_str));
	ret = atcacert_date_dec_posix_uint32_be(NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}




TEST_GROUP(atcacert_date_dec_posix_uint32_le);

TEST_SETUP(atcacert_date_dec_posix_uint32_le)
{
}

TEST_TEAR_DOWN(atcacert_date_dec_posix_uint32_le)
{
}

TEST(atcacert_date_dec_posix_uint32_le, good)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE] = { 0xF7, 0x4C, 0x7F, 0x52 };
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_dec_posix_uint32_le(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_le, min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE] = { 0x00, 0x00, 0x00, 0x00 };
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 1970, 1, 1, 0, 0, 0);

	ret = atcacert_date_dec_posix_uint32_le(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_le, int32_max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE] = { 0xFF, 0xFF, 0xFF, 0x7F };
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2038, 1, 19, 3, 14, 7);

	ret = atcacert_date_dec_posix_uint32_le(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_le, large)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE] = { 0xFB, 0xFC, 0xFD, 0xFE };
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2105, 7, 26, 13, 30, 35);

	ret = atcacert_date_dec_posix_uint32_le(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_le, max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE] = { 0xFE, 0xFF, 0xFF, 0xFF };
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2106, 2, 7, 6, 28, 14);

	ret = atcacert_date_dec_posix_uint32_le(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_le, bad_params)
{
	int ret = 0;
	uint8_t ts_str_good[DATEFMT_POSIX_UINT32_LE_SIZE] = { 0xF7, 0x4C, 0x7F, 0x52 };
	uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE];
	atcacert_tm_utc_t ts;

	memcpy(ts_str, ts_str_good, sizeof(ts_str));
	ret = atcacert_date_dec_posix_uint32_le(NULL, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	memcpy(ts_str, ts_str_good, sizeof(ts_str));
	ret = atcacert_date_dec_posix_uint32_le(ts_str, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	memcpy(ts_str, ts_str_good, sizeof(ts_str));
	ret = atcacert_date_dec_posix_uint32_le(NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}



TEST_GROUP(atcacert_date_dec_rfc5280_gen);

TEST_SETUP(atcacert_date_dec_rfc5280_gen)
{
}

TEST_TEAR_DOWN(atcacert_date_dec_rfc5280_gen)
{
}

TEST(atcacert_date_dec_rfc5280_gen, good)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1] = "20141211100908Z";
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 2014, 12, 11, 10, 9, 8);

	ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_gen, min)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1] = "00000101000000Z";
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 0, 1, 1, 0, 0, 0);

	ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_gen, max)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1] = "99991231235959Z";
	atcacert_tm_utc_t ts_ref;
	atcacert_tm_utc_t ts;

	set_tm(&ts_ref, 9999, 12, 31, 23, 59, 59);

	ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_gen, bad_int)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
	atcacert_tm_utc_t ts;

	memcpy(ts_str, "A0141211100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "2014A211100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "201412A1100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "20141211A00908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "2014121110A908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);

	memcpy(ts_str, "201412111009A8Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_date_dec_rfc5280_gen, bad_params)
{
	int ret = 0;
	uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
	atcacert_tm_utc_t ts;

	memcpy(ts_str, "20141211100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_gen(NULL, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	memcpy(ts_str, "20141211100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_gen(ts_str, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	memcpy(ts_str, "20141211100908Z", sizeof(ts_str));
	ret = atcacert_date_dec_rfc5280_gen(NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}



TEST_GROUP(atcacert_date_get_max_date);

TEST_SETUP(atcacert_date_get_max_date)
{
}

TEST_TEAR_DOWN(atcacert_date_get_max_date)
{
}

TEST(atcacert_date_get_max_date, iso8601_sep)
{
	int ret = ATCACERT_E_SUCCESS;
	atcacert_tm_utc_t ts;
	atcacert_tm_utc_t ts_ref;

	set_tm(&ts_ref, 9999, 12, 31, 23, 59, 59);

	ret = atcacert_date_get_max_date(DATEFMT_ISO8601_SEP, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_get_max_date, rfc5280_utc)
{
	int ret = ATCACERT_E_SUCCESS;
	atcacert_tm_utc_t ts;
	atcacert_tm_utc_t ts_ref;

	set_tm(&ts_ref, 2049, 12, 31, 23, 59, 59);

	ret = atcacert_date_get_max_date(DATEFMT_RFC5280_UTC, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_get_max_date, posix_uint32_be)
{
	int ret = ATCACERT_E_SUCCESS;
	atcacert_tm_utc_t ts;
	atcacert_tm_utc_t ts_ref;

	set_tm(&ts_ref, 2106, 2, 7, 6, 28, 15);

	ret = atcacert_date_get_max_date(DATEFMT_POSIX_UINT32_BE, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_get_max_date, posix_uint32_le)
{
	int ret = ATCACERT_E_SUCCESS;
	atcacert_tm_utc_t ts;
	atcacert_tm_utc_t ts_ref;

	set_tm(&ts_ref, 2106, 2, 7, 6, 28, 15);

	ret = atcacert_date_get_max_date(DATEFMT_POSIX_UINT32_LE, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_get_max_date, rfc5280_gen)
{
	int ret = ATCACERT_E_SUCCESS;
	atcacert_tm_utc_t ts;
	atcacert_tm_utc_t ts_ref;

	set_tm(&ts_ref, 9999, 12, 31, 23, 59, 59);

	ret = atcacert_date_get_max_date(DATEFMT_RFC5280_GEN, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_get_max_date, new_format)
{
	int ret = ATCACERT_E_SUCCESS;
	atcacert_tm_utc_t ts;

	ret = atcacert_date_get_max_date(DATEFMT_RFC5280_GEN + 1, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}

TEST(atcacert_date_get_max_date, bad_params)
{
	int ret = ATCACERT_E_SUCCESS;
	atcacert_tm_utc_t ts;

	ret = atcacert_date_get_max_date(-1, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_get_max_date(DATEFMT_ISO8601_SEP, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_get_max_date(-1, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}



TEST_GROUP(atcacert_date_dec_compcert);

TEST_SETUP(atcacert_date_dec_compcert)
{
}

TEST_TEAR_DOWN(atcacert_date_dec_compcert)
{
}

TEST(atcacert_date_dec_compcert, good)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date, issue_date_ref;
	atcacert_tm_utc_t expire_date, expire_date_ref;
	uint8_t enc_dates[3] = { 0xA9, 0x9D, 0x5C };

	set_tm(&issue_date_ref,  2021,      3, 7, 10, 0, 0);
	set_tm(&expire_date_ref, 2021 + 28, 3, 7, 10, 0, 0);

	ret = atcacert_date_dec_compcert(enc_dates, DATEFMT_RFC5280_GEN, &issue_date, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&issue_date_ref, &issue_date, sizeof(issue_date));
	TEST_ASSERT_EQUAL_MEMORY(&expire_date_ref, &expire_date, sizeof(expire_date));
}

TEST(atcacert_date_dec_compcert, min)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date, issue_date_ref;
	atcacert_tm_utc_t expire_date, expire_date_ref;
	uint8_t enc_dates[3] = { 0x00, 0x84, 0x00 };

	set_tm(&issue_date_ref,  2000, 1, 1, 0, 0, 0);
	set_tm(&expire_date_ref, 9999, 12, 31, 23, 59, 59);

	ret = atcacert_date_dec_compcert(enc_dates, DATEFMT_RFC5280_GEN, &issue_date, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&issue_date_ref, &issue_date, sizeof(issue_date));
	TEST_ASSERT_EQUAL_MEMORY(&expire_date_ref, &expire_date, sizeof(expire_date));
}

TEST(atcacert_date_dec_compcert, max)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date, issue_date_ref;
	atcacert_tm_utc_t expire_date, expire_date_ref;
	uint8_t enc_dates[3] = { 0xFE, 0x7E, 0xFF };

	set_tm(&issue_date_ref,  2031,      12, 31, 23, 0, 0);
	set_tm(&expire_date_ref, 2031 + 31, 12, 31, 23, 0, 0);

	ret = atcacert_date_dec_compcert(enc_dates, DATEFMT_RFC5280_GEN, &issue_date, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&issue_date_ref, &issue_date, sizeof(issue_date));
	TEST_ASSERT_EQUAL_MEMORY(&expire_date_ref, &expire_date, sizeof(expire_date));
}

TEST(atcacert_date_dec_compcert, posix_uint32_be)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date, issue_date_ref;
	atcacert_tm_utc_t expire_date, expire_date_ref;
	uint8_t enc_dates[3] = { 0x00, 0x84, 0x00 };

	set_tm(&issue_date_ref, 2000, 1, 1, 0, 0, 0);
	set_tm(&expire_date_ref, 2106, 2, 7, 6, 28, 15);

	ret = atcacert_date_dec_compcert(enc_dates, DATEFMT_POSIX_UINT32_BE, &issue_date, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&issue_date_ref, &issue_date, sizeof(issue_date));
	TEST_ASSERT_EQUAL_MEMORY(&expire_date_ref, &expire_date, sizeof(expire_date));
}

TEST(atcacert_date_dec_compcert, bad_params)
{
	int ret = 0;
	atcacert_tm_utc_t issue_date;
	atcacert_tm_utc_t expire_date;
	uint8_t enc_dates[3] = { 0xA9, 0x9D, 0x5C };

	ret = atcacert_date_dec_compcert(NULL, DATEFMT_RFC5280_GEN, &issue_date, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(enc_dates, -1, &issue_date, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(NULL, -1, &issue_date, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(enc_dates, DATEFMT_RFC5280_GEN, NULL, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(NULL, DATEFMT_RFC5280_GEN, NULL, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(enc_dates, -1, NULL, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(NULL, -1, NULL, &expire_date);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(enc_dates, DATEFMT_RFC5280_GEN, &issue_date, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(NULL, DATEFMT_RFC5280_GEN, &issue_date, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(enc_dates, -1, &issue_date, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(NULL, -1, &issue_date, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(enc_dates, DATEFMT_RFC5280_GEN, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(NULL, DATEFMT_RFC5280_GEN, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(enc_dates, -1, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec_compcert(NULL, -1, NULL, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}





TEST_GROUP(atcacert_date_dec);

TEST_SETUP(atcacert_date_dec)
{
}

TEST_TEAR_DOWN(atcacert_date_dec)
{
}

TEST(atcacert_date_dec, iso8601_sep)
{
	int ret = 0;
	const uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1] = "2013-11-10T09:08:07Z";
	size_t ts_str_size = sizeof(ts_str);
	atcacert_tm_utc_t ts;
	atcacert_tm_utc_t ts_ref;

	set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_dec(DATEFMT_ISO8601_SEP, ts_str, ts_str_size, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec, rfc5280_utc)
{
	int ret = 0;
	const uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1] = "131110090807Z";
	size_t ts_str_size = sizeof(ts_str);
	atcacert_tm_utc_t ts;
	atcacert_tm_utc_t ts_ref;

	set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_dec(DATEFMT_RFC5280_UTC, ts_str, ts_str_size, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec, posix_uint32_be)
{
	int ret = 0;
	const uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0x52, 0x7F, 0x4C, 0xF7 };
	size_t ts_str_size = sizeof(ts_str);
	atcacert_tm_utc_t ts;
	atcacert_tm_utc_t ts_ref;

	set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_dec(DATEFMT_POSIX_UINT32_BE, ts_str, ts_str_size, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec, posix_uint32_le)
{
	int ret = 0;
	const uint8_t ts_str[DATEFMT_POSIX_UINT32_LE_SIZE] = { 0xF7, 0x4C, 0x7F, 0x52 };
	size_t ts_str_size = sizeof(ts_str);
	atcacert_tm_utc_t ts;
	atcacert_tm_utc_t ts_ref;

	set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_dec(DATEFMT_POSIX_UINT32_LE, ts_str, ts_str_size, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec, rfc5280_gen)
{
	int ret = 0;
	const uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1] = "20131110090807Z";
	size_t ts_str_size = sizeof(ts_str);
	atcacert_tm_utc_t ts;
	atcacert_tm_utc_t ts_ref;

	set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

	ret = atcacert_date_dec(DATEFMT_RFC5280_GEN, ts_str, ts_str_size, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
	TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec, small_buf)
{
	int ret = 0;
	const uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1] = "131110090807Z";
	size_t ts_str_size = sizeof(ts_str) - 2;
	atcacert_tm_utc_t ts;

	ret = atcacert_date_dec(DATEFMT_RFC5280_UTC, ts_str, ts_str_size, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_date_dec, bad_format)
{
	int ret = 0;
	const uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
	size_t ts_str_size = sizeof(ts_str);
	atcacert_tm_utc_t ts;

	ret = atcacert_date_dec(-1, ts_str, ts_str_size, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ret = atcacert_date_dec(DATEFMT_RFC5280_GEN + 1, ts_str, ts_str_size, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}

TEST(atcacert_date_dec, bad_params)
{
	int ret = 0;
	const uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1] = "20131110090807Z";
	size_t ts_str_size = sizeof(ts_str);
	atcacert_tm_utc_t ts;

	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_dec(DATEFMT_RFC5280_GEN, NULL, ts_str_size, &ts);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_dec(DATEFMT_RFC5280_GEN, ts_str, ts_str_size, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

	ts_str_size = sizeof(ts_str);
	ret = atcacert_date_dec(DATEFMT_RFC5280_GEN, NULL, ts_str_size, NULL);
	TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}