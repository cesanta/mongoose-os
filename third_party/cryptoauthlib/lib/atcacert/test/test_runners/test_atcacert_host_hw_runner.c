#include "test/unity.h"
#include "test/unity_fixture.h"

#ifdef __GNUC__
// Unity macros trigger this warning
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif

TEST_GROUP_RUNNER(atcacert_host_hw)
{
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_cert_hw);
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_cert_hw_verify_failed);
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_cert_hw_short_cert);
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_cert_hw_bad_sig);
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_cert_hw_bad_params);
    
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_gen_challenge_hw);
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_gen_challenge_hw_bad_params);
    
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw);
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw_bad_challenge);
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw_bad_response);
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw_bad_public_key);
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw_malformed_public_key);
    RUN_TEST_CASE(atcacert_host_hw, atcacert_host_hw__atcacert_verify_response_hw_bad_params);
}
