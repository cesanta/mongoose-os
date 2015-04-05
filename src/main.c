/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "fossa.h"
#include "openssl/ssl.h"
#include "v7.h"

void init_crypto(struct v7 *v7);
void init_socket(struct v7 *v7);
void init_os(struct v7 *v7);
void init_file(struct v7 *v7);

void init_smartjs(struct v7 *v7) {
  init_crypto(v7);
  init_socket(v7);
  init_os(v7);
  init_file(v7);
}

#ifndef UNIT_TEST
int main(int argc, char *argv[]) {
  return v7_main(argc, argv, init_smartjs);
}
#endif
