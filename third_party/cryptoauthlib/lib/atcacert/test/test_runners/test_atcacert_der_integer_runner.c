#include "test/unity.h"
#include "test/unity_fixture.h"

#ifdef __GNUC__
// Unity macros trigger this warning
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif

TEST_GROUP_RUNNER(atcacert_der_enc_integer)
{
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__signed_min);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__signed_1byte);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__signed_multi_byte);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__signed_large);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__signed_trim_1_pos);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__signed_trim_multi_pos);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__signed_trim_all_pos);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__signed_trim_1_neg);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__signed_trim_multi_neg);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__signed_trim_all_neg);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__unsigned_min);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__unsigned_min_pad);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__unsigned_multi_byte);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__unsigned_multi_byte_pad);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__unsigned_large);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__unsigned_large_pad);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__unsigned_trim_1_pos);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__unsigned_trim_multi_pos);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__unsigned_trim_all_pos);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__unsigned_trim_neg_pad);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__small_buf);
    RUN_TEST_CASE(atcacert_der_enc_integer, atcacert_der_enc_integer__bad_params);
}

TEST_GROUP_RUNNER(atcacert_der_dec_integer)
{
    RUN_TEST_CASE(atcacert_der_dec_integer, atcacert_der_dec_integer__good);
    RUN_TEST_CASE(atcacert_der_dec_integer, atcacert_der_dec_integer__good_large);
    RUN_TEST_CASE(atcacert_der_dec_integer, atcacert_der_dec_integer__zero_size);
    RUN_TEST_CASE(atcacert_der_dec_integer, atcacert_der_dec_integer__not_enough_data);
    RUN_TEST_CASE(atcacert_der_dec_integer, atcacert_der_dec_integer__small_buf);
    RUN_TEST_CASE(atcacert_der_dec_integer, atcacert_der_dec_integer__bad_params);
}