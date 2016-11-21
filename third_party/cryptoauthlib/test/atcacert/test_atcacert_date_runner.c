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

#undef min
#undef max

TEST_GROUP_RUNNER(atcacert_date_enc_iso8601_sep)
{
	RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, good);
	RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, min);
	RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, max);
	RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, bad_year);
	RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, bad_month);
	RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, bad_day);
	RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, bad_hour);
	RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, bad_min);
	RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, bad_sec);
	RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc_rfc5280_utc)
{
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, good);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, min);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, max);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, y2k);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, bad_year);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, bad_month);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, bad_day);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, bad_hour);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, bad_min);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, bad_sec);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc_posix_uint32_be)
{
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, good);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, min);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, large);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, max);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, bad_low);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, bad_high);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc_posix_uint32_le)
{
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_le, good);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_le, min);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_le, large);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_le, max);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_le, bad_low);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_le, bad_high);
	RUN_TEST_CASE(atcacert_date_enc_posix_uint32_le, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc_rfc5280_gen)
{
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, good);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, min);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, max);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, bad_year);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, bad_month);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, bad_day);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, bad_hour);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, bad_min);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, bad_sec);
	RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc_compcert)
{
	RUN_TEST_CASE(atcacert_date_enc_compcert, good);
	RUN_TEST_CASE(atcacert_date_enc_compcert, min);
	RUN_TEST_CASE(atcacert_date_enc_compcert, max);
	RUN_TEST_CASE(atcacert_date_enc_compcert, bad_year);
	RUN_TEST_CASE(atcacert_date_enc_compcert, bad_month);
	RUN_TEST_CASE(atcacert_date_enc_compcert, bad_day);
	RUN_TEST_CASE(atcacert_date_enc_compcert, bad_hour);
	RUN_TEST_CASE(atcacert_date_enc_compcert, bad_expire);
	RUN_TEST_CASE(atcacert_date_enc_compcert, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc)
{
	RUN_TEST_CASE(atcacert_date_enc, iso8601_sep);
	RUN_TEST_CASE(atcacert_date_enc, rfc5280_utc);
	RUN_TEST_CASE(atcacert_date_enc, posix_uint32_be);
	RUN_TEST_CASE(atcacert_date_enc, posix_uint32_le);
	RUN_TEST_CASE(atcacert_date_enc, rfc5280_gen);
	RUN_TEST_CASE(atcacert_date_enc, small_buf);
	RUN_TEST_CASE(atcacert_date_enc, bad_format);
	RUN_TEST_CASE(atcacert_date_enc, bad_params);
}


TEST_GROUP_RUNNER(atcacert_date_dec_iso8601_sep)
{
	RUN_TEST_CASE(atcacert_date_dec_iso8601_sep, good);
	RUN_TEST_CASE(atcacert_date_dec_iso8601_sep, min);
	RUN_TEST_CASE(atcacert_date_dec_iso8601_sep, max);
	RUN_TEST_CASE(atcacert_date_dec_iso8601_sep, bad_int);
	RUN_TEST_CASE(atcacert_date_dec_iso8601_sep, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec_rfc5280_utc)
{
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, good);
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, min);
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, max);
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, y2k);
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, bad_int);
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec_posix_uint32_be)
{
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, good);
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, min);
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, int32_max);
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, large);
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, max);
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec_posix_uint32_le)
{
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_le, good);
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_le, min);
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_le, int32_max);
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_le, large);
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_le, max);
	RUN_TEST_CASE(atcacert_date_dec_posix_uint32_le, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec_rfc5280_gen)
{
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_gen, good);
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_gen, min);
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_gen, max);
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_gen, bad_int);
	RUN_TEST_CASE(atcacert_date_dec_rfc5280_gen, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_get_max_date)
{
	RUN_TEST_CASE(atcacert_date_get_max_date, iso8601_sep);
	RUN_TEST_CASE(atcacert_date_get_max_date, rfc5280_utc);
	RUN_TEST_CASE(atcacert_date_get_max_date, posix_uint32_be);
	RUN_TEST_CASE(atcacert_date_get_max_date, posix_uint32_le);
	RUN_TEST_CASE(atcacert_date_get_max_date, rfc5280_gen);
	RUN_TEST_CASE(atcacert_date_get_max_date, new_format);
	RUN_TEST_CASE(atcacert_date_get_max_date, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec_compcert)
{
	RUN_TEST_CASE(atcacert_date_dec_compcert, good);
	RUN_TEST_CASE(atcacert_date_dec_compcert, min);
	RUN_TEST_CASE(atcacert_date_dec_compcert, max);
	RUN_TEST_CASE(atcacert_date_dec_compcert, posix_uint32_be);
	RUN_TEST_CASE(atcacert_date_dec_compcert, bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec)
{
	RUN_TEST_CASE(atcacert_date_dec, iso8601_sep);
	RUN_TEST_CASE(atcacert_date_dec, rfc5280_utc);
	RUN_TEST_CASE(atcacert_date_dec, posix_uint32_be);
	RUN_TEST_CASE(atcacert_date_dec, posix_uint32_le);
	RUN_TEST_CASE(atcacert_date_dec, rfc5280_gen);
	RUN_TEST_CASE(atcacert_date_dec, small_buf);
	RUN_TEST_CASE(atcacert_date_dec, bad_format);
	RUN_TEST_CASE(atcacert_date_dec, bad_params);
}