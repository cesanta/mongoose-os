/** \brief cert date tests
*
* Copyright (c) 2015 Atmel Corporation. All rights reserved.
*
* \asf_license_start
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
*    Atmel microcontroller product.
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
* \asf_license_stop
 */ 



#include "atcacert/atcacert_date.h"
#include "test/unity.h"
#include "test/unity_fixture.h"
#include <string.h>

static void set_tm(struct tm* ts, int year, int month, int day, int hour, int min, int sec)
{
    memset(ts, 0, sizeof(struct tm));

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

TEST(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_good)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
    const char ts_str_ref[sizeof(ts_str)+1] = "2013-11-10T09:08:07Z";
    struct tm ts;
    set_tm(&ts, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = "0000-01-01T00:00:00Z";
    struct tm ts;
    set_tm(&ts, 0, 1, 1, 0, 0, 0);

    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_max)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = "9999-12-31T23:59:59Z";
    struct tm ts;
    set_tm(&ts, 9999, 12, 31, 23, 59, 59);

    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_year)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
    struct tm ts;

    set_tm(&ts, -1, 11, 10, 9, 8, 7);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 10000, 11, 10, 9, 8, 7);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_month)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 0, 10, 9, 8, 7);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 13, 10, 9, 8, 7);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_day)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 0, 9, 8, 7);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 32, 9, 8, 7);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_hour)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 10, -1, 8, 7);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 10, 24, 8, 7);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 10, 9, -1, 7);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 10, 9, 60, 7);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_sec)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 10, 9, 8, -1);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 10, 9, 8, 60);
    ret = atcacert_date_enc_iso8601_sep(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_params)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE];
    struct tm ts;

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

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_good)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = "131110090807Z";
    struct tm ts;
    set_tm(&ts, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = "500101000000Z";
    struct tm ts;
    set_tm(&ts, 1950, 1, 1, 0, 0, 0);

    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_max)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = "491231235959Z";
    struct tm ts;
    set_tm(&ts, 2049, 12, 31, 23, 59, 59);

    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_y2k)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    char ts_str_ref[sizeof(ts_str) + 1];
    struct tm ts;

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

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_year)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    struct tm ts;

    set_tm(&ts, 1949, 11, 10, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2050, 11, 10, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_month)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 0, 10, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 13, 10, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_day)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 0, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 32, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_hour)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 10, -1, 8, 7);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 10, 24, 8, 7);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 10, 9, -1, 7);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 10, 9, 60, 7);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_sec)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 10, 9, 8, -1);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 10, 9, 8, 60);
    ret = atcacert_date_enc_rfc5280_utc(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_params)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE];
    struct tm ts;

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

TEST(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_good)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = {0x52, 0x7F, 0x4C, 0xF7};
    struct tm ts;
    set_tm(&ts, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = { 0x00, 0x00, 0x00, 0x00 };
    struct tm ts;
    set_tm(&ts, 1970, 1, 1, 0, 0, 0);

    ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_large)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = { 0xFE, 0xFD, 0xFC, 0xFB };
    struct tm ts;
    set_tm(&ts, 2105, 7, 26, 13, 30, 35);

    ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_max)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
    struct tm ts;
    const char ts_str_ref[sizeof(ts_str) + 1] = { 0xFF, 0xFF, 0xFF, 0xFE };
    set_tm(&ts, 2106, 2, 7, 6, 28, 14);

    ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_bad_low)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
    struct tm ts;

    set_tm(&ts, 1969, 12, 31, 23, 59, 59);
    ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_bad_high)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
    struct tm ts;

    set_tm(&ts, 2106, 2, 7, 6, 28, 15);
    ret = atcacert_date_enc_posix_uint32_be(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_bad_params)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
    struct tm ts;

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



TEST_GROUP(atcacert_date_enc_rfc5280_gen);

TEST_SETUP(atcacert_date_enc_rfc5280_gen)
{
}

TEST_TEAR_DOWN(atcacert_date_enc_rfc5280_gen)
{
}

TEST(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_good)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = "20131110090807Z";
    struct tm ts;
    set_tm(&ts, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = "00000101000000Z";
    struct tm ts;
    set_tm(&ts, 0, 1, 1, 0, 0, 0);

    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_max)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
    const char ts_str_ref[sizeof(ts_str) + 1] = "99991231235959Z";
    struct tm ts;
    set_tm(&ts, 9999, 12, 31, 23, 59, 59);

    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(ts_str_ref, ts_str, sizeof(ts_str));
}

TEST(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_year)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
    struct tm ts;

    set_tm(&ts, -1, 11, 10, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 10000, 11, 10, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_month)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 0, 10, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 13, 10, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_day)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 0, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 32, 9, 8, 7);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_hour)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 10, -1, 8, 7);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 10, 24, 8, 7);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 10, 9, -1, 7);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 10, 9, 60, 7);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_sec)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
    struct tm ts;

    set_tm(&ts, 2013, 11, 10, 9, 8, -1);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);

    set_tm(&ts, 2013, 11, 10, 9, 8, 60);
    ret = atcacert_date_enc_rfc5280_gen(&ts, ts_str);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_params)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE];
    struct tm ts;

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

TEST(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_good)
{
    int ret = 0;
    struct tm issue_date;
    uint8_t enc_dates[3];
    uint8_t enc_dates_ref[sizeof(enc_dates)] = { 0xA9, 0x9D, 0x5C };
    uint8_t expire_years = 28;
    set_tm(&issue_date, 2021, 3, 7, 10, 0, 0);

    ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(enc_dates_ref, enc_dates, sizeof(enc_dates));
}

TEST(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_min)
{
    int ret = 0;
    struct tm issue_date;
    uint8_t enc_dates[3];
    uint8_t enc_dates_ref[sizeof(enc_dates)] = { 0x00, 0x84, 0x00 };
    uint8_t expire_years = 0;
    set_tm(&issue_date, 2000, 1, 1, 00, 00, 00);

    ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(enc_dates_ref, enc_dates, sizeof(enc_dates));
}

TEST(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_max)
{
    int ret = 0;
    struct tm issue_date;
    uint8_t enc_dates[3];
    uint8_t enc_dates_ref[sizeof(enc_dates)] = { 0xFE, 0x7E, 0xFF };
    uint8_t expire_years = 31;
    set_tm(&issue_date, 2031, 12, 31, 23, 00, 00);

    ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(enc_dates_ref, enc_dates, sizeof(enc_dates));
}

TEST(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_year)
{
    int ret = 0;
    struct tm issue_date;
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

TEST(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_month)
{
    int ret = 0;
    struct tm issue_date;
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

TEST(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_day)
{
    int ret = 0;
    struct tm issue_date;
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

TEST(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_hour)
{
    int ret = 0;
    struct tm issue_date;
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

TEST(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_expire)
{
    int ret = 0;
    struct tm issue_date;
    uint8_t enc_dates[3];
    uint8_t expire_years = 0;

    expire_years = 32;
    set_tm(&issue_date, 2021, 3, 7, 10, 0, 0);
    ret = atcacert_date_enc_compcert(&issue_date, expire_years, enc_dates);
    TEST_ASSERT_EQUAL(ATCACERT_E_INVALID_DATE, ret);
}

TEST(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_params)
{
    int ret = 0;
    struct tm issue_date;
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

TEST(atcacert_date_enc, atcacert_date__atcacert_date_enc_iso8601_sep)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE+1];
    size_t ts_str_size = sizeof(ts_str);
    const char ts_str_ref[sizeof(ts_str)] = "2013-11-10T09:08:07Z";
    struct tm ts;
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

TEST(atcacert_date_enc, atcacert_date__atcacert_date_enc_rfc5280_utc)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1];
    size_t ts_str_size = sizeof(ts_str);
    const char ts_str_ref[sizeof(ts_str)] = "131110090807Z";
    struct tm ts;
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

TEST(atcacert_date_enc, atcacert_date__atcacert_date_enc_posix_uint32_be)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE + 1];
    size_t ts_str_size = sizeof(ts_str);
    const char ts_str_ref[sizeof(ts_str)-1] = { 0x52, 0x7F, 0x4C, 0xF7 };
    struct tm ts;
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

TEST(atcacert_date_enc, atcacert_date__atcacert_date_enc_rfc5280_gen)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
    size_t ts_str_size = sizeof(ts_str);
    const char ts_str_ref[sizeof(ts_str)] = "20131110090807Z";
    struct tm ts;
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

TEST(atcacert_date_enc, atcacert_date__atcacert_date_enc_small_buf)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE - 1];
    size_t ts_str_size = sizeof(ts_str);
    struct tm ts;
    set_tm(&ts, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_enc(DATEFMT_RFC5280_UTC, &ts, ts_str, &ts_str_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_BUFFER_TOO_SMALL, ret);
    TEST_ASSERT_EQUAL(DATEFMT_RFC5280_UTC_SIZE, ts_str_size);
}

TEST(atcacert_date_enc, atcacert_date__atcacert_date_enc_bad_format)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
    size_t ts_str_size = sizeof(ts_str);
    struct tm ts;
    set_tm(&ts, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_enc(100, &ts, ts_str, &ts_str_size);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}

TEST(atcacert_date_enc, atcacert_date__atcacert_date_enc_bad_params)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
    size_t ts_str_size = sizeof(ts_str);
    struct tm ts;
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

TEST(atcacert_date_dec_iso8601_sep, atcacert_date__atcacert_date_dec_iso8601_sep_good)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1] = "2014-12-11T10:09:08Z";
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 2014, 12, 11, 10, 9, 8);

    ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_iso8601_sep, atcacert_date__atcacert_date_dec_iso8601_sep_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1] = "0000-01-01T00:00:00Z";
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 0, 1, 1, 0, 0, 0);

    ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_iso8601_sep, atcacert_date__atcacert_date_dec_iso8601_sep_max)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1] = "9999-12-31T23:59:59Z";
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 9999, 12, 31, 23, 59, 59);

    ret = atcacert_date_dec_iso8601_sep(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_iso8601_sep, atcacert_date__atcacert_date_dec_iso8601_sep_bad_int)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1];
    struct tm ts;

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

TEST(atcacert_date_dec_iso8601_sep, atcacert_date__atcacert_date_dec_iso8601_sep_bad_params)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1];
    struct tm ts;

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

TEST(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_good)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1] = "141211100908Z";
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 2014, 12, 11, 10, 9, 8);

    ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1] = "500101000000Z";
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 1950, 1, 1, 0, 0, 0);

    ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_max)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1] = "491231235959Z";
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 2049, 12, 31, 23, 59, 59);

    ret = atcacert_date_dec_rfc5280_utc(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_y2k)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1];
    struct tm ts_ref;
    struct tm ts;

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

TEST(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_bad_int)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1];
    struct tm ts;

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

TEST(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_bad_params)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1];
    struct tm ts;

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

TEST(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_good)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0x52, 0x7F, 0x4C, 0xF7 };
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_dec_posix_uint32_be(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    ts.tm_wday = 0;
    ts.tm_yday = 0;
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0x00, 0x00, 0x00, 0x00 };
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 1970, 1, 1, 0, 0, 0);

    ret = atcacert_date_dec_posix_uint32_be(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    ts.tm_wday = 0;
    ts.tm_yday = 0;
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_int32_max)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0x7F, 0xFF, 0xFF, 0xFF };
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 2038, 1, 19, 3, 14, 7);

    ret = atcacert_date_dec_posix_uint32_be(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    ts.tm_wday = 0;
    ts.tm_yday = 0;
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_large)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0xFE, 0xFD, 0xFC, 0xFB };
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 2105, 7, 26, 13, 30, 35);

    ret = atcacert_date_dec_posix_uint32_be(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    ts.tm_wday = 0;
    ts.tm_yday = 0;
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_max)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0xFF, 0xFF, 0xFF, 0xFE };
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 2106, 2, 7, 6, 28, 14);

    ret = atcacert_date_dec_posix_uint32_be(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    ts.tm_wday = 0;
    ts.tm_yday = 0;
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_bad_params)
{
    int ret = 0;
    uint8_t ts_str_good[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0x52, 0x7F, 0x4C, 0xF7 };
    uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE];
    struct tm ts;

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



TEST_GROUP(atcacert_date_dec_rfc5280_gen);

TEST_SETUP(atcacert_date_dec_rfc5280_gen)
{
}

TEST_TEAR_DOWN(atcacert_date_dec_rfc5280_gen)
{
}

TEST(atcacert_date_dec_rfc5280_gen, atcacert_date__atcacert_date_dec_rfc5280_gen_good)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1] = "20141211100908Z";
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 2014, 12, 11, 10, 9, 8);

    ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_gen, atcacert_date__atcacert_date_dec_rfc5280_gen_min)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1] = "00000101000000Z";
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 0, 1, 1, 0, 0, 0);

    ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_gen, atcacert_date__atcacert_date_dec_rfc5280_gen_max)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1] = "99991231235959Z";
    struct tm ts_ref;
    struct tm ts;
    set_tm(&ts_ref, 9999, 12, 31, 23, 59, 59);

    ret = atcacert_date_dec_rfc5280_gen(ts_str, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec_rfc5280_gen, atcacert_date__atcacert_date_dec_rfc5280_gen_bad_int)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
    struct tm ts;

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

TEST(atcacert_date_dec_rfc5280_gen, atcacert_date__atcacert_date_dec_rfc5280_gen_bad_params)
{
    int ret = 0;
    uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
    struct tm ts;

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




TEST_GROUP(atcacert_date_dec_compcert);

TEST_SETUP(atcacert_date_dec_compcert)
{
}

TEST_TEAR_DOWN(atcacert_date_dec_compcert)
{
}

TEST(atcacert_date_dec_compcert, atcacert_date__atcacert_date_dec_compcert_good)
{
    int ret = 0;
    struct tm issue_date, issue_date_ref;
    struct tm expire_date, expire_date_ref;
    uint8_t enc_dates[3] = { 0xA9, 0x9D, 0x5C };
    set_tm(&issue_date_ref,  2021,      3, 7, 10, 0, 0);
    set_tm(&expire_date_ref, 2021 + 28, 3, 7, 10, 0, 0);

    ret = atcacert_date_dec_compcert(enc_dates, &issue_date, &expire_date);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&issue_date_ref, &issue_date, sizeof(issue_date));
    TEST_ASSERT_EQUAL_MEMORY(&expire_date_ref, &expire_date, sizeof(expire_date));
}

TEST(atcacert_date_dec_compcert, atcacert_date__atcacert_date_dec_compcert_min)
{
    int ret = 0;
    struct tm issue_date, issue_date_ref;
    struct tm expire_date, expire_date_ref;
    uint8_t enc_dates[3] = { 0x00, 0x84, 0x00 };
    set_tm(&issue_date_ref,  2000, 1, 1, 0, 0, 0);
    set_tm(&expire_date_ref, 9999, 12, 31, 23, 59, 59);

    ret = atcacert_date_dec_compcert(enc_dates, &issue_date, &expire_date);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&issue_date_ref, &issue_date, sizeof(issue_date));
    TEST_ASSERT_EQUAL_MEMORY(&expire_date_ref, &expire_date, sizeof(expire_date));
}

TEST(atcacert_date_dec_compcert, atcacert_date__atcacert_date_dec_compcert_max)
{
    int ret = 0;
    struct tm issue_date, issue_date_ref;
    struct tm expire_date, expire_date_ref;
    uint8_t enc_dates[3] = { 0xFE, 0x7E, 0xFF };
    set_tm(&issue_date_ref,  2031,      12, 31, 23, 0, 0);
    set_tm(&expire_date_ref, 2031 + 31, 12, 31, 23, 0, 0);

    ret = atcacert_date_dec_compcert(enc_dates, &issue_date, &expire_date);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&issue_date_ref, &issue_date, sizeof(issue_date));
    TEST_ASSERT_EQUAL_MEMORY(&expire_date_ref, &expire_date, sizeof(expire_date));
}

TEST(atcacert_date_dec_compcert, atcacert_date__atcacert_date_dec_compcert_bad_params)
{
    int ret = 0;
    struct tm issue_date;
    struct tm expire_date;
    uint8_t enc_dates[3] = { 0xA9, 0x9D, 0x5C };

    ret = atcacert_date_dec_compcert(NULL, &issue_date, &expire_date);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

    ret = atcacert_date_dec_compcert(enc_dates, NULL, &expire_date);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

    ret = atcacert_date_dec_compcert(NULL, NULL, &expire_date);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

    ret = atcacert_date_dec_compcert(enc_dates, &issue_date, NULL);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

    ret = atcacert_date_dec_compcert(NULL, &issue_date, NULL);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

    ret = atcacert_date_dec_compcert(enc_dates, NULL, NULL);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);

    ret = atcacert_date_dec_compcert(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}





TEST_GROUP(atcacert_date_dec);

TEST_SETUP(atcacert_date_dec)
{
}

TEST_TEAR_DOWN(atcacert_date_dec)
{
}

TEST(atcacert_date_dec, atcacert_date__atcacert_date_dec_iso8601_sep)
{
    int ret = 0;
    const uint8_t ts_str[DATEFMT_ISO8601_SEP_SIZE + 1] = "2013-11-10T09:08:07Z";
    size_t ts_str_size = sizeof(ts_str);
    struct tm ts;
    struct tm ts_ref;
    set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_dec(DATEFMT_ISO8601_SEP, ts_str, ts_str_size, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec, atcacert_date__atcacert_date_dec_rfc5280_utc)
{
    int ret = 0;
    const uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1] = "131110090807Z";
    size_t ts_str_size = sizeof(ts_str);
    struct tm ts;
    struct tm ts_ref;
    set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_dec(DATEFMT_RFC5280_UTC, ts_str, ts_str_size, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec, atcacert_date__atcacert_date_dec_posix_uint32_be)
{
    int ret = 0;
    const uint8_t ts_str[DATEFMT_POSIX_UINT32_BE_SIZE] = { 0x52, 0x7F, 0x4C, 0xF7 };
    size_t ts_str_size = sizeof(ts_str);
    struct tm ts;
    struct tm ts_ref;
    set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_dec(DATEFMT_POSIX_UINT32_BE, ts_str, ts_str_size, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    ts.tm_wday = 0;
    ts.tm_yday = 0;
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec, atcacert_date__atcacert_date_dec_rfc5280_gen)
{
    int ret = 0;
    const uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1] = "20131110090807Z";
    size_t ts_str_size = sizeof(ts_str);
    struct tm ts;
    struct tm ts_ref;
    set_tm(&ts_ref, 2013, 11, 10, 9, 8, 7);

    ret = atcacert_date_dec(DATEFMT_RFC5280_GEN, ts_str, ts_str_size, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_SUCCESS, ret);
    TEST_ASSERT_EQUAL_MEMORY(&ts_ref, &ts, sizeof(ts));
}

TEST(atcacert_date_dec, atcacert_date__atcacert_date_dec_small_buf)
{
    int ret = 0;
    const uint8_t ts_str[DATEFMT_RFC5280_UTC_SIZE + 1] = "131110090807Z";
    size_t ts_str_size = sizeof(ts_str) - 2;
    struct tm ts;

    ret = atcacert_date_dec(DATEFMT_RFC5280_UTC, ts_str, ts_str_size, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_DECODING_ERROR, ret);
}

TEST(atcacert_date_dec, atcacert_date__atcacert_date_dec_bad_format)
{
    int ret = 0;
    const uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1];
    size_t ts_str_size = sizeof(ts_str);
    struct tm ts;

    ret = atcacert_date_dec(100, ts_str, ts_str_size, &ts);
    TEST_ASSERT_EQUAL(ATCACERT_E_BAD_PARAMS, ret);
}

TEST(atcacert_date_dec, atcacert_date__atcacert_date_dec_bad_params)
{
    int ret = 0;
    const uint8_t ts_str[DATEFMT_RFC5280_GEN_SIZE + 1] = "20131110090807Z";
    size_t ts_str_size = sizeof(ts_str);
    struct tm ts;

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