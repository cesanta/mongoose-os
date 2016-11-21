#include "test/unity.h"
#include "test/unity_fixture.h"

#ifdef __GNUC__
// Unity macros trigger this warning
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif

TEST_GROUP_RUNNER(atcacert_der_enc_ecdsa_sig_value)
{
    RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, atcacert_der_enc_ecdsa_sig_value__no_padding);
    RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, atcacert_der_enc_ecdsa_sig_value__r_padding);
    RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, atcacert_der_enc_ecdsa_sig_value__s_padding);
    RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, atcacert_der_enc_ecdsa_sig_value__rs_padding);
    RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, atcacert_der_enc_ecdsa_sig_value__trim);
    RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, atcacert_der_enc_ecdsa_sig_value__trim_all);
    RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, atcacert_der_enc_ecdsa_sig_value__small_buf);
    RUN_TEST_CASE(atcacert_der_enc_ecdsa_sig_value, atcacert_der_enc_ecdsa_sig_value__bad_params);
}

TEST_GROUP_RUNNER(atcacert_der_dec_ecdsa_sig_value)
{
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__no_padding);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__r_padding);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__s_padding);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__rs_padding);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__trim);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__trim_all);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_bs_tag);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_bs_length_low);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_bs_length_high);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_bs_extra_data);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_bs_spare_bits);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_seq_tag);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_seq_length_low);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_seq_length_high);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_seq_extra_data);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_rint_tag);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_rint_length_low);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_rint_length_high);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_sint_tag);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_sint_length_low);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_sint_length_high);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_rint_too_large);
    RUN_TEST_CASE(atcacert_der_dec_ecdsa_sig_value, atcacert_der_dec_ecdsa_sig_value__bad_sint_too_large);
}