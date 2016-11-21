#include "test/unity.h"
#include "test/unity_fixture.h"

#ifdef __GNUC__
// Unity macros trigger this warning
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif

TEST_GROUP_RUNNER(atcacert_der_enc_length)
{
    RUN_TEST_CASE(atcacert_der_enc_length, atcacert_der_enc_length__short_form);
    RUN_TEST_CASE(atcacert_der_enc_length, atcacert_der_enc_length__long_form_2byte);
    RUN_TEST_CASE(atcacert_der_enc_length, atcacert_der_enc_length__long_form_3byte);
    RUN_TEST_CASE(atcacert_der_enc_length, atcacert_der_enc_length__long_form_4byte);
    RUN_TEST_CASE(atcacert_der_enc_length, atcacert_der_enc_length__long_form_5byte);
    RUN_TEST_CASE(atcacert_der_enc_length, atcacert_der_enc_length__small_buf);
    RUN_TEST_CASE(atcacert_der_enc_length, atcacert_der_enc_length__bad_params);
}

TEST_GROUP_RUNNER(atcacert_der_dec_length)
{
    RUN_TEST_CASE(atcacert_der_dec_length, atcacert_der_dec_der_length__good);
    RUN_TEST_CASE(atcacert_der_dec_length, atcacert_der_dec_der_length__zero_size);
    RUN_TEST_CASE(atcacert_der_dec_length, atcacert_der_dec_der_length__not_enough_data);
    RUN_TEST_CASE(atcacert_der_dec_length, atcacert_der_dec_der_length__indefinite_form);
    RUN_TEST_CASE(atcacert_der_dec_length, atcacert_der_dec_der_length__too_large);
    RUN_TEST_CASE(atcacert_der_dec_length, atcacert_der_dec_der_length__bad_params);
}