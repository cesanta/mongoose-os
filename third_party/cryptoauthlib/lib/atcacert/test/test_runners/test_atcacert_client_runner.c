#include "test/unity.h"
#include "test/unity_fixture.h"

#ifdef __GNUC__
// Unity macros trigger this warning
#pragma GCC diagnostic ignored "-Wnested-externs"
#endif

TEST_GROUP_RUNNER(atcacert_client)
{
    // Load certificate data onto the device
    RUN_TEST_CASE(atcacert_client, atcacert_client__init);
    
	RUN_TEST_CASE(atcacert_client, atcacert_client__atcacert_read_cert_signer);
    RUN_TEST_CASE(atcacert_client, atcacert_client__atcacert_read_cert_device);
    RUN_TEST_CASE(atcacert_client, atcacert_client__atcacert_read_cert_small_buf);
    RUN_TEST_CASE(atcacert_client, atcacert_client__atcacert_read_cert_bad_params);
    
    RUN_TEST_CASE(atcacert_client, atcacert_client__atcacert_get_response);
    RUN_TEST_CASE(atcacert_client, atcacert_client__atcacert_get_response_bad_params);
}
