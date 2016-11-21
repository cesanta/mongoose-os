#include "test/unity.h"
#include "test/unity_fixture.h"

#ifdef __GNUC__
// Unity macros trigger this warning
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif

TEST_GROUP_RUNNER(atcacert_date_enc_iso8601_sep)
{
    RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_good);
    RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_min);
    RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_max);
    RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_year);
    RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_month);
    RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_day);
    RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_hour);
    RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_min);
    RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_sec);
    RUN_TEST_CASE(atcacert_date_enc_iso8601_sep, atcacert_date__atcacert_date_enc_iso8601_sep_bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc_rfc5280_utc)
{
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_good);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_min);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_max);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_y2k);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_year);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_month);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_day);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_hour);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_min);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_sec);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_utc, atcacert_date__atcacert_date_enc_rfc5280_utc_bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc_posix_uint32_be)
{
    RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_good);
    RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_min);
    RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_large);
    RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_max);
    RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_bad_low);
    RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_bad_high);
    RUN_TEST_CASE(atcacert_date_enc_posix_uint32_be, atcacert_date__atcacert_date_enc_posix_uint32_be_bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc_rfc5280_gen)
{
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_good);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_min);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_max);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_year);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_month);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_day);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_hour);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_min);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_sec);
    RUN_TEST_CASE(atcacert_date_enc_rfc5280_gen, atcacert_date__atcacert_date_enc_rfc5280_gen_bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc_compcert)
{
    RUN_TEST_CASE(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_good);
    RUN_TEST_CASE(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_min);
    RUN_TEST_CASE(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_max);
    RUN_TEST_CASE(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_year);
    RUN_TEST_CASE(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_month);
    RUN_TEST_CASE(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_day);
    RUN_TEST_CASE(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_hour);
    RUN_TEST_CASE(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_expire);
    RUN_TEST_CASE(atcacert_date_enc_compcert, atcacert_date__atcacert_date_enc_compcert_bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_enc)
{
    RUN_TEST_CASE(atcacert_date_enc, atcacert_date__atcacert_date_enc_iso8601_sep);
    RUN_TEST_CASE(atcacert_date_enc, atcacert_date__atcacert_date_enc_rfc5280_utc);
    RUN_TEST_CASE(atcacert_date_enc, atcacert_date__atcacert_date_enc_posix_uint32_be);
    RUN_TEST_CASE(atcacert_date_enc, atcacert_date__atcacert_date_enc_rfc5280_gen);
    RUN_TEST_CASE(atcacert_date_enc, atcacert_date__atcacert_date_enc_small_buf);
    RUN_TEST_CASE(atcacert_date_enc, atcacert_date__atcacert_date_enc_bad_format);
    RUN_TEST_CASE(atcacert_date_enc, atcacert_date__atcacert_date_enc_bad_params);
}


TEST_GROUP_RUNNER(atcacert_date_dec_iso8601_sep)
{
    RUN_TEST_CASE(atcacert_date_dec_iso8601_sep, atcacert_date__atcacert_date_dec_iso8601_sep_good);
    RUN_TEST_CASE(atcacert_date_dec_iso8601_sep, atcacert_date__atcacert_date_dec_iso8601_sep_min);
    RUN_TEST_CASE(atcacert_date_dec_iso8601_sep, atcacert_date__atcacert_date_dec_iso8601_sep_max);
    RUN_TEST_CASE(atcacert_date_dec_iso8601_sep, atcacert_date__atcacert_date_dec_iso8601_sep_bad_int);
    RUN_TEST_CASE(atcacert_date_dec_iso8601_sep, atcacert_date__atcacert_date_dec_iso8601_sep_bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec_rfc5280_utc)
{
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_good);
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_min);
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_max);
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_y2k);
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_bad_int);
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_utc, atcacert_date__atcacert_date_dec_rfc5280_utc_bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec_posix_uint32_be)
{
    RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_good);
    RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_min);
    RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_int32_max);
    RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_large);
    RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_max);
    RUN_TEST_CASE(atcacert_date_dec_posix_uint32_be, atcacert_date__atcacert_date_dec_posix_uint32_be_bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec_rfc5280_gen)
{
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_gen, atcacert_date__atcacert_date_dec_rfc5280_gen_good);
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_gen, atcacert_date__atcacert_date_dec_rfc5280_gen_min);
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_gen, atcacert_date__atcacert_date_dec_rfc5280_gen_max);
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_gen, atcacert_date__atcacert_date_dec_rfc5280_gen_bad_int);
    RUN_TEST_CASE(atcacert_date_dec_rfc5280_gen, atcacert_date__atcacert_date_dec_rfc5280_gen_bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec_compcert)
{
    RUN_TEST_CASE(atcacert_date_dec_compcert, atcacert_date__atcacert_date_dec_compcert_good);
    RUN_TEST_CASE(atcacert_date_dec_compcert, atcacert_date__atcacert_date_dec_compcert_min);
    RUN_TEST_CASE(atcacert_date_dec_compcert, atcacert_date__atcacert_date_dec_compcert_max);
    RUN_TEST_CASE(atcacert_date_dec_compcert, atcacert_date__atcacert_date_dec_compcert_bad_params);
}

TEST_GROUP_RUNNER(atcacert_date_dec)
{
    RUN_TEST_CASE(atcacert_date_dec, atcacert_date__atcacert_date_dec_iso8601_sep);
    RUN_TEST_CASE(atcacert_date_dec, atcacert_date__atcacert_date_dec_rfc5280_utc);
    RUN_TEST_CASE(atcacert_date_dec, atcacert_date__atcacert_date_dec_posix_uint32_be);
    RUN_TEST_CASE(atcacert_date_dec, atcacert_date__atcacert_date_dec_rfc5280_gen);
    RUN_TEST_CASE(atcacert_date_dec, atcacert_date__atcacert_date_dec_small_buf);
    RUN_TEST_CASE(atcacert_date_dec, atcacert_date__atcacert_date_dec_bad_format);
    RUN_TEST_CASE(atcacert_date_dec, atcacert_date__atcacert_date_dec_bad_params);
}