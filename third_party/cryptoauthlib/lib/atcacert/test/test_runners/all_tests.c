#include "test/unity_fixture.h"

#ifdef __GNUC__
// Unity macros trigger this warning
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif

void RunAllCertDataTests(void)
{
    RUN_TEST_GROUP(atcacert_der_enc_length);
    RUN_TEST_GROUP(atcacert_der_dec_length);

    RUN_TEST_GROUP(atcacert_der_enc_integer);
    RUN_TEST_GROUP(atcacert_der_dec_integer);

    RUN_TEST_GROUP(atcacert_der_enc_ecdsa_sig_value);
    RUN_TEST_GROUP(atcacert_der_dec_ecdsa_sig_value);

    RUN_TEST_GROUP(atcacert_date_enc_iso8601_sep);
    RUN_TEST_GROUP(atcacert_date_enc_rfc5280_utc);
    RUN_TEST_GROUP(atcacert_date_enc_posix_uint32_be);
    RUN_TEST_GROUP(atcacert_date_enc_rfc5280_gen);
    RUN_TEST_GROUP(atcacert_date_enc_compcert);
    RUN_TEST_GROUP(atcacert_date_enc);

    RUN_TEST_GROUP(atcacert_date_dec_iso8601_sep);
    RUN_TEST_GROUP(atcacert_date_dec_rfc5280_utc);
    RUN_TEST_GROUP(atcacert_date_dec_posix_uint32_be);
    RUN_TEST_GROUP(atcacert_date_dec_rfc5280_gen);
    RUN_TEST_GROUP(atcacert_date_dec_compcert);
    RUN_TEST_GROUP(atcacert_date_dec);

    RUN_TEST_GROUP(atcacert_def);
}

void RunAllCertIOTests(void)
{
	RUN_TEST_GROUP(atcacert_client);
    RUN_TEST_GROUP(atcacert_host_hw);
}
