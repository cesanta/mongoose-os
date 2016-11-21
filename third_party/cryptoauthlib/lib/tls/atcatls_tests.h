/*
 * atca_tls_tests.h
 *
 */ 


#ifndef ATCA_TLS_TESTS_H_
#define ATCA_TLS_TESTS_H_


#include "test/unity.h"

void atcatls_test_runner(void);

void test_atcatls_config_device(void);

void test_atcatls_init_finish(void);

void test_atcatls_create_key(void);
void test_atcatls_sign(void);
void test_atcatls_verify(void);
void test_atcatls_ecdh(void);
void test_atcatls_ecdhe(void);
void test_atcatls_random(void);
void test_atcatls_get_sn(void);

void test_atcatls_get_cert(void);

void test_atcatls_init_enc_key(void);
void test_atcatls_enc_write_read(void);

#endif /* ATCA_TLS_TESTS_H_ */