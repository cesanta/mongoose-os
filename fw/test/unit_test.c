/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "test_util.h"
#include "miot_config.h"
#include "cs_dbg.h"
#include "cs_file.h"
#include "sys_conf.h"

static const char *test_config(void) {
  size_t size;
  char *json1 = cs_read_file(".build/sys_conf_defaults.json", &size);
  char *json2 = cs_read_file("data/overrides.json", &size);
  const struct miot_conf_entry *schema = sys_conf_schema();
  struct sys_conf conf;

  memset(&conf, 0, sizeof(conf));
  ASSERT(json1 != NULL);
  ASSERT(json2 != NULL);
  cs_log_set_level(LL_NONE);

  /* Load defaults */
  ASSERT_EQ(miot_conf_parse(mg_mk_str(json1), "*", schema, &conf), true);
  ASSERT_EQ(conf.wifi.ap.channel, 6);
  ASSERT_STREQ(conf.wifi.ap.pass, "Elduderino");
  ASSERT(conf.wifi.sta.ssid == NULL);
  ASSERT(conf.wifi.sta.pass == NULL);
  ASSERT(conf.debug.level == 2);
  ASSERT_EQ(conf.http.port, 80);   /* integer */
  ASSERT_EQ(conf.http.enable, 1);  /* boolean */
  ASSERT_STREQ(conf.wifi.ap.dhcp_end, "192.168.4.200");

  /* Apply overrides */
  ASSERT_EQ(miot_conf_parse(mg_mk_str(json2), "*", schema, &conf), true);
  ASSERT_STREQ(conf.wifi.sta.ssid, "cookadoodadoo");   /* Set string */
  ASSERT_STREQ(conf.wifi.sta.pass, "try less cork");
  ASSERT_EQ(conf.debug.level, 1);    /* Override integer */
  ASSERT(conf.wifi.ap.pass == NULL); /* Reset string - set to NULL */
  ASSERT_EQ(conf.http.enable, 0);    /* Override boolean */

  free(json1);
  free(json2);

  return NULL;
}

static const char *run_tests(const char *filter, double *total_elapsed) {
  RUN_TEST(test_config);
  return NULL;
}

int __cdecl main(int argc, char *argv[]) {
  const char *fail_msg;
  const char *filter = argc > 1 ? argv[1] : "";
  double total_elapsed = 0.0;

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  fail_msg = run_tests(filter, &total_elapsed);
  printf("%s, run %d in %.3lfs\n", fail_msg ? "FAIL" : "PASS", num_tests,
         total_elapsed);
  return fail_msg == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}

