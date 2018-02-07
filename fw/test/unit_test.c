/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "cs_dbg.h"
#include "cs_file.h"

#include "frozen/frozen.h"

#include "mgos_config_util.h"
#include "mgos_event.h"

#include "sys_conf.h"
#include "test_main.h"
#include "test_util.h"

static const char *test_config(void) {
  size_t size;
  char *json1 = cs_read_file(".build/sys_conf_defaults.json", &size);
  char *json2 = cs_read_file("data/overrides.json", &size);
  const struct mgos_conf_entry *schema = sys_conf_schema();
  struct sys_conf conf;

  memset(&conf, 0, sizeof(conf));
  ASSERT(json1 != NULL);
  ASSERT(json2 != NULL);
  cs_log_set_level(LL_NONE);

  /* Load defaults */
  ASSERT_EQ(mgos_conf_parse(mg_mk_str(json1), "*", schema, &conf), true);
  ASSERT_EQ(conf.wifi.ap.channel, 6);
  ASSERT_STREQ(conf.wifi.ap.pass, "Elduderino");
  ASSERT(conf.wifi.sta.ssid == NULL);
  ASSERT(conf.wifi.sta.pass == NULL);
  ASSERT(conf.debug.level == 2);
  ASSERT_EQ(conf.http.port, 80);  /* integer */
  ASSERT_EQ(conf.http.enable, 1); /* boolean */
  ASSERT_STREQ(conf.wifi.ap.dhcp_end, "192.168.4.200");

  /* Apply overrides */
  ASSERT_EQ(mgos_conf_parse(mg_mk_str(json2), "*", schema, &conf), true);
  ASSERT_STREQ(conf.wifi.sta.ssid, "cookadoodadoo"); /* Set string */
  ASSERT_STREQ(conf.wifi.sta.pass, "try less cork");
  ASSERT_EQ(conf.debug.level, 1);    /* Override integer */
  ASSERT(conf.wifi.ap.pass == NULL); /* Reset string - set to NULL */
  ASSERT_EQ(conf.http.enable, 0);    /* Override boolean */

  free(json1);
  free(json2);

  return NULL;
}

static const char *test_json_scanf(void) {
  int a = 0;
  bool b = false;
  int c = 0xFFFFFFFF;
  const char *str = "{\"b\":true,\"c\":false,\"a\":2}";
  ASSERT(json_scanf(str, strlen(str), "{a:%d, b:%B, c:%B}", &a, &b, &c) == 3);
  ASSERT(a == 2);
  ASSERT(b == true);
  if (sizeof(bool) == 1)
    ASSERT((char) c == false);
  else
    ASSERT(c == false);

  return NULL;
}

#define GRP1 MGOS_EVENT_BASE('G', '0', '1')
#define GRP2 MGOS_EVENT_BASE('G', '0', '2')
#define GRP3 MGOS_EVENT_BASE('G', '0', '3')

enum grp1_ev {
  GRP1_EV0 = GRP1,
  GRP1_EV1,
  GRP1_EV2,
};

enum grp2_ev {
  GRP2_EV0 = GRP2,
  GRP2_EV1,
  GRP2_EV2,
};

enum grp3_ev {
  GRP3_EV0 = GRP3,
  GRP3_EV1,
  GRP3_EV2,
};

#define EV_FLAG_GRP1_EV0 (1 << 0)
#define EV_FLAG_GRP1_EV1 (1 << 1)
#define EV_FLAG_GRP1_EV2 (1 << 2)

#define EV_FLAG_GRP2_EV0 (1 << 3)
#define EV_FLAG_GRP2_EV1 (1 << 4)
#define EV_FLAG_GRP2_EV2 (1 << 5)

#define EV_FLAG_GRP3_EV0 (1 << 6)
#define EV_FLAG_GRP3_EV1 (1 << 7)
#define EV_FLAG_GRP3_EV2 (1 << 8)

static void ev_cb(int ev, void *ev_data, void *userdata) {
  uint32_t *pflags = (uint32_t *) userdata;
  uint32_t flag = 0;

  switch (ev) {
    case GRP1_EV0:
      flag = EV_FLAG_GRP1_EV0;
      break;
    case GRP1_EV1:
      flag = EV_FLAG_GRP1_EV1;
      break;
    case GRP1_EV2:
      flag = EV_FLAG_GRP1_EV2;
      break;

    case GRP2_EV0:
      flag = EV_FLAG_GRP2_EV0;
      break;
    case GRP2_EV1:
      flag = EV_FLAG_GRP2_EV1;
      break;
    case GRP2_EV2:
      flag = EV_FLAG_GRP2_EV2;
      break;

    case GRP3_EV0:
      flag = EV_FLAG_GRP3_EV0;
      break;
    case GRP3_EV1:
      flag = EV_FLAG_GRP3_EV1;
      break;
    case GRP3_EV2:
      flag = EV_FLAG_GRP3_EV2;
      break;
  }

  *pflags |= flag;

  (void) ev_data;
  (void) userdata;
}

static const char *test_events(void) {
  ASSERT(mgos_event_register_base(GRP1, "grp1") == true);
  ASSERT(mgos_event_register_base(GRP2, "grp2") == true);
  ASSERT(mgos_event_register_base(GRP3, "grp3") == true);

  uint32_t flags1 = 0;
  uint32_t flags2 = 0;
  uint32_t flags3 = 0;

  ASSERT(mgos_event_add_handler(GRP1_EV1, ev_cb, &flags1));
  ASSERT(mgos_event_add_handler(GRP1_EV2, ev_cb, &flags1));
  ASSERT(mgos_event_add_handler(GRP2_EV2, ev_cb, &flags1));

  ASSERT(mgos_event_add_group_handler(GRP2_EV1, ev_cb, &flags2));

  ASSERT(mgos_event_add_handler(GRP3_EV0, ev_cb, &flags3));
  ASSERT(mgos_event_add_group_handler(GRP3_EV0, ev_cb, &flags3));

  flags1 = flags2 = flags3 = 0;
  mgos_event_trigger(GRP1_EV0, NULL);
  ASSERT_EQ(flags1, 0);
  ASSERT_EQ(flags2, 0);
  ASSERT_EQ(flags3, 0);

  flags1 = flags2 = flags3 = 0;
  mgos_event_trigger(GRP1_EV1, NULL);
  ASSERT_EQ(flags1, EV_FLAG_GRP1_EV1);
  ASSERT_EQ(flags2, 0);
  ASSERT_EQ(flags3, 0);

  flags1 = flags2 = flags3 = 0;
  mgos_event_trigger(GRP1_EV2, NULL);
  ASSERT_EQ(flags1, EV_FLAG_GRP1_EV2);
  ASSERT_EQ(flags2, 0);
  ASSERT_EQ(flags3, 0);

  flags1 = flags2 = flags3 = 0;
  mgos_event_trigger(GRP2_EV0, NULL);
  ASSERT_EQ(flags1, 0);
  ASSERT_EQ(flags2, EV_FLAG_GRP2_EV0);
  ASSERT_EQ(flags3, 0);

  flags1 = flags2 = flags3 = 0;
  mgos_event_trigger(GRP2_EV1, NULL);
  ASSERT_EQ(flags1, 0);
  ASSERT_EQ(flags2, EV_FLAG_GRP2_EV1);
  ASSERT_EQ(flags3, 0);

  flags1 = flags2 = flags3 = 0;
  mgos_event_trigger(GRP2_EV2, NULL);
  ASSERT_EQ(flags1, EV_FLAG_GRP2_EV2);
  ASSERT_EQ(flags2, EV_FLAG_GRP2_EV2);
  ASSERT_EQ(flags3, 0);

  return NULL;
}

void tests_setup(void) {
}

const char *tests_run(const char *filter) {
  RUN_TEST(test_config);
  RUN_TEST(test_json_scanf);
  RUN_TEST(test_events);
  return NULL;
}

void tests_teardown(void) {
}
