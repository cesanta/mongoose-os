/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/cs_time.h"
#include "mgos_cron.h"
#include "mgos_timers.h"
#include "mgos_location.h"

#define FAIL(str, line)                           \
  do {                                            \
    printf("Fail on line %d: [%s]\n", line, str); \
    return str;                                   \
  } while (0)

#define ASSERT(expr)                    \
  do {                                  \
    s_num_tests++;                      \
    if (!(expr)) FAIL(#expr, __LINE__); \
  } while (0)

#define RUN_TEST(test)        \
  do {                        \
    const char *msg = test(); \
    if (msg) return msg;      \
  } while (0)

#define TEST_BASE_TIME "2017-08-29 22:00:20"

/* TEST_BASE_TIME is 2017-08-29 22:00:20 UTC+-X */
static struct test_entry s_te[] = {/* Local time */
                                   {.expr = "* * * * * *",
                                    .expected[0] = "2017-08-29 22:00:21",
                                    .expected[1] = "2017-08-29 22:00:22",
                                    .expected[2] = "2017-08-29 22:00:23"},
                                   {.expr = "0 * * * * *",
                                    .expected[0] = "2017-08-29 22:01:00",
                                    .expected[1] = "2017-08-29 22:02:00",
                                    .expected[2] = "2017-08-29 22:03:00"},
                                   {.expr = "0 0 * * * *",
                                    .expected[0] = "2017-08-29 23:00:00",
                                    .expected[1] = "2017-08-30 00:00:00",
                                    .expected[2] = "2017-08-30 01:00:00"},
                                   {.expr = "0 0 0 * * *",
                                    .expected[0] = "2017-08-30 00:00:00",
                                    .expected[1] = "2017-08-31 00:00:00",
                                    .expected[2] = "2017-09-01 00:00:00"},
                                   {.expr = "0 0 0 1 * *",
                                    .expected[0] = "2017-09-01 00:00:00",
                                    .expected[1] = "2017-10-01 00:00:00",
                                    .expected[2] = "2017-11-01 00:00:00"},
                                   {.expr = "10 * * * * *",
                                    .expected[0] = "2017-08-29 22:01:10",
                                    .expected[1] = "2017-08-29 22:02:10",
                                    .expected[2] = "2017-08-29 22:03:10"},
                                   {.expr = "10 */1 * * * *",
                                    .expected[0] = "2017-08-29 22:01:10",
                                    .expected[1] = "2017-08-29 22:02:10",
                                    .expected[2] = "2017-08-29 22:03:10"},
                                   {.expr = "*/15 * * * * *",
                                    .expected[0] = "2017-08-29 22:00:30",
                                    .expected[1] = "2017-08-29 22:00:45",
                                    .expected[2] = "2017-08-29 22:01:00"},
                                   {.expr = "* * * 5 9 *",
                                    .expected[0] = "2017-09-05 00:00:00",
                                    .expected[1] = "2017-09-05 00:00:01",
                                    .expected[2] = "2017-09-05 00:00:02"},
                                   {.expr = "0 0 0 31 * *",
                                    .expected[0] = "2017-08-31 00:00:00",
                                    .expected[1] = "2017-10-31 00:00:00",
                                    .expected[2] = "2017-12-31 00:00:00"},
                                   {.expr = "0 0 0 * * 3",
                                    .expected[0] = "2017-08-30 00:00:00",
                                    .expected[1] = "2017-09-06 00:00:00",
                                    .expected[2] = "2017-09-13 00:00:00"},
                                   {.expr = "0 * * * * 3",
                                    .expected[0] = "2017-08-30 00:00:00",
                                    .expected[1] = "2017-08-30 00:01:00",
                                    .expected[2] = "2017-08-30 00:02:00"},
                                   {.expr = "0 0 9 1-7 * 1",
                                    .expected[0] = "2017-09-04 09:00:00",
                                    .expected[1] = "2017-10-02 09:00:00",
                                    .expected[2] = "2017-11-06 09:00:00"}};
struct sorted_test_entry {
  int id;
  time_t expected;
};

extern long timezone;

static char s_tz[8] = {0};
static int s_num_tests = 0;
static char s_str_time[32] = {0};
static struct tm s_tm = {0};
static struct sorted_test_entry *s_sorted_te = NULL;
static int s_current_id = 0;

static void s_get_timezone(void) {
  tzset();
  int t = timezone / 60 / 60;
  if (t == 0) {
    strcpy(s_tz, "UTC");
  } else {
    bool neg = false;
    if (t < 0) {
      neg = true;
      t *= -1;
    }
    sprintf(s_tz, "UTC%c%d", neg ? '+' : '-', t);
  }
}

static struct tm *s_cron_test_str2tm(const char *date) {
  memset(&s_tm, 0, sizeof s_tm);

  sscanf(date, "%04d-%02d-%02d %02d:%02d:%02d", &s_tm.tm_year, &s_tm.tm_mon,
         &s_tm.tm_mday, &s_tm.tm_hour, &s_tm.tm_min, &s_tm.tm_sec);
  s_tm.tm_mon -= 1;
  s_tm.tm_year -= 1900;
  s_tm.tm_isdst = -1;

  return &s_tm;
}

static time_t s_cron_test_str2timeloc(const char *date) {
  return mktime(s_cron_test_str2tm(date));
}

static time_t s_cron_test_str2timegm(const char *date) {
  return (time_t) cs_timegm(s_cron_test_str2tm(date));
}

static char *s_cron_test_tm2str(struct tm *t, const char *gmt) {
  size_t sz = ARRAY_SIZE(s_str_time);
  memset(s_str_time, 0, sz);
  snprintf(s_str_time, sz - 1, "%04d-%02d-%02d %02d:%02d:%02d %s",
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min,
           t->tm_sec, gmt);
  return s_str_time;
}

static char *s_cron_test_timeloc2str(time_t date) {
  struct tm t;
  localtime_r(&date, &t);
  return s_cron_test_tm2str(&t, s_tz);
}

static char *s_cron_test_timegm2str(time_t date) {
  struct tm t;
  gmtime_r(&date, &t);
  return s_cron_test_tm2str(&t, "UTC");
}

static void s_cron_test_expr_order_cb(void *user_data, mgos_cron_id_t id) {
  struct test_entry *te = (struct test_entry *) user_data;
  time_t expected = s_cron_test_str2timeloc(te->expected[0]);

  printf(
      "expr = \"%s\"\n"
      "current = %lu (%s)\n"
      "expe[0] = %lu (%s)",
      te->expr, te->current, s_cron_test_timeloc2str(te->current), expected,
      s_cron_test_timeloc2str(expected));

  if (te->current == expected && s_sorted_te[s_current_id++].id == te->id) {
    te->count++;
  } else {
    printf("      <---- FAIL");
  }
  mgos_cron_remove(id);
  printf("\n--------------------------------------------------\n");
}

static int s_cmp(const void *p1, const void *p2) {
  time_t res = ((struct sorted_test_entry *) p1)->expected -
               ((struct sorted_test_entry *) p2)->expected;
  int ret;
  if (res > 0) {
    ret = 1;
  } else if (res == 0) {
    ret = 0;
  } else {
    ret = -1;
  }
  return ret;
}

static const char *s_cron_test_expr_order(void) {
  int sz = ARRAY_SIZE(s_te), i;
  time_t t = s_cron_test_str2timeloc(TEST_BASE_TIME);
  mgos_set_time(t);
  time_t ct = (time_t) mg_time();
  printf(
      "\nNOW: %lu (%s)\n"
      "--------------------------------------------------\n",
      ct, s_cron_test_timeloc2str(ct));

  if ((s_sorted_te = (struct sorted_test_entry *) calloc(
           sz, sizeof(*s_sorted_te))) == NULL) {
    return NULL;
  }
  for (i = 0; i < sz; i++) {
    s_te[i].current = 0;
    s_te[i].count = 0;
    s_sorted_te[i].id = s_te[i].id = i;
    s_sorted_te[i].expected = s_cron_test_str2timeloc(s_te[i].expected[0]);
  }
  qsort(s_sorted_te, sz, sizeof(*s_sorted_te), s_cmp);

  for (i = 0; i < sz; i++) {
    ASSERT(mgos_cron_add(s_te[i].expr, s_cron_test_expr_order_cb, &s_te[i]) !=
           MGOS_INVALID_CRON_ID);
  }
  mgos_schedule_timers(s_te, sz);
  free(s_sorted_te);

  printf("\n");
  for (i = 0; i < sz; i++) {
    ASSERT(s_te[i].count == 1);
  }
  return NULL;
}

static void s_cron_test_shedule_cb(void *user_data, mgos_cron_id_t id) {
  struct test_entry *te = (struct test_entry *) user_data;
  int count = te->count;
  time_t expected = s_cron_test_str2timeloc(te->expected[count]);

  printf(
      "expr = \"%s\"\n"
      "current = %lu (%s)\n"
      "expe[%d] = %lu (%s)",
      te->expr, te->current, s_cron_test_timeloc2str(te->current), count,
      expected, s_cron_test_timeloc2str(expected));

  if (te->current == expected) {
    te->count++;
  } else {
    printf("      <---- FAIL");
  }

  if (count == te->count || te->count == 3) {
    mgos_cron_remove(id);
    te->cron_entry = NULL;
  }
  printf("\n--------------------------------------------------\n");
}

static const char *s_cron_test_shedule(void) {
  int sz = ARRAY_SIZE(s_te);
  time_t t = s_cron_test_str2timeloc(TEST_BASE_TIME);
  mgos_set_time(t);
  time_t ct = (time_t) mg_time();
  printf(
      "\nNOW: %lu (%s)\n"
      "--------------------------------------------------\n",
      ct, s_cron_test_timeloc2str(ct));

  for (int i = 0; i < sz; i++) {
    s_te[i].current = 0;
    s_te[i].count = 0;
    ASSERT(mgos_cron_add(s_te[i].expr, s_cron_test_shedule_cb, &s_te[i]) !=
           MGOS_INVALID_CRON_ID);
  }
  mgos_schedule_timers(s_te, sz);

  printf("\n");
  for (int i = 0; i < sz; i++) {
    ASSERT(s_te[i].count == 3);
  }
  return NULL;
}

struct mgos_location_lat_lon s_loc[] = {
    {.lat = 55.750000, .lon = 37.580000},  /* Moscow */
    {.lat = 35.083955, .lon = -106.6333},  /* Albuquerque */
    {.lat = -33.87000, .lon = 151.22000}}; /* Sydnay */

/* NOW is 2017-08-29 22:00:20 UTC

Sunrise:
  2017-08-29 02:29:00 UTC Moscow
  2017-08-29 12:37:00 UTC Albuquerque
  2017-08-29 20:16:00 UTC Sydnay
Sunset:
  2017-08-29 16:31:00 UTC Moscow
  2017-08-29 01:38:00 UTC Albuquerque
  2017-08-29 07:35:00 UTC Sydnay

  Times from https://www.esrl.noaa.gov/gmd/grad/solcalc
*/
static struct test_entry s_sun_te[] =
    {/* UTC */
     {.expr = "@sunrise",
      .expected[0] = "2017-08-30 02:31:00",  /* Moscow */
      .expected[1] = "2017-08-30 12:38:00",  /* Albuquerque */
      .expected[2] = "2017-08-30 20:15:00"}, /* Sydnay */
     {.expr = "@sunrise+4h1m10s",
      .expected[0] = "2017-08-30 06:32:00",
      .expected[1] = "2017-08-30 16:39:00",
      .expected[2] = "2017-08-30 00:17:00"},
     {.expr = "@sunrise-3h16m",
      .expected[0] = "2017-08-29 23:15:00",
      .expected[1] = "2017-08-30 09:22:00",
      .expected[2] = "2017-08-30 16:59:00"},
     {.expr = "@sunset",
      .expected[0] = "2017-08-30 16:29:00",
      .expected[1] = "2017-08-30 01:37:00",
      .expected[2] = "2017-08-30 07:36:00"},
     {.expr = "@sunset+10h0m",
      .expected[0] = "2017-08-30 02:32:00",
      .expected[1] = "2017-08-30 11:37:00",
      .expected[2] = "2017-08-30 17:36:00"},
     {.expr = "@sunset-3h1m",
      .expected[0] = "2017-08-30 13:28:00",
      .expected[1] = "2017-08-29 22:36:00",
      .expected[2] = "2017-08-30 04:35:00"}};

static void s_cron_test_sun_cb(void *user_data, mgos_cron_id_t id) {
  struct test_entry *te = (struct test_entry *) user_data;
  time_t expected = s_cron_test_str2timegm(te->expected[te->count]);

  /* The expected time is rounded, so we need to round
     the calculated time as well */
  struct tm t;
  time_t date = te->current;
  gmtime_r(&date, &t);
  if (t.tm_sec > 30) t.tm_min += 1;
  t.tm_sec = 0;
  date = (time_t) cs_timegm(&t);

  printf(
      "expr: \"%s\"\n"
      "current: %lu (%s)\n"
      "expe[%d]: %lu (%s)",
      te->expr, date, s_cron_test_timegm2str(date), te->count, expected,
      s_cron_test_timegm2str(expected));

  if (date == expected) {
    te->count = 3;
  } else {
    printf("      <---- FAIL");
  }
  mgos_cron_remove(id);
  printf("\n--------------------------------------------------\n");
}

static const char *s_cron_test_sun(void) {
  int i, j;
  int loc_sz = ARRAY_SIZE(s_loc);
  int sun_te_sz = ARRAY_SIZE(s_sun_te);

  for (j = 0; j < loc_sz; j++) {
    time_t t = s_cron_test_str2timegm(TEST_BASE_TIME);
    mgos_set_time(t);

    location_set(s_loc[j].lat, s_loc[j].lon);

    time_t ct = (time_t) mg_time();
    struct mgos_location_lat_lon loc;
    mgos_location_get(&loc);
    printf(
        "\nNOW: %lu (%s)\n"
        "LAT: %f LON: %f\n"
        "--------------------------------------------------\n",
        ct, s_cron_test_timegm2str(ct), loc.lat, loc.lon);

    for (i = 0; i < sun_te_sz; i++) {
      s_sun_te[i].current = 0;
      s_sun_te[i].count = j;
      ASSERT(mgos_cron_add(s_sun_te[i].expr, s_cron_test_sun_cb,
                           &s_sun_te[i]) != MGOS_INVALID_CRON_ID);
    }
    mgos_schedule_timers(s_sun_te, sun_te_sz);

    printf("\n");
    for (i = 0; i < sun_te_sz; i++) {
      ASSERT(s_sun_te[i].count == 3);
    }
  }
  return NULL;
}

static const char *s_run_all_tests(void) {
  RUN_TEST(s_cron_test_expr_order);
  RUN_TEST(s_cron_test_shedule);
  RUN_TEST(s_cron_test_sun);
  return NULL;
}

int main(void) {
  s_get_timezone();
  const char *fail_msg = s_run_all_tests();
  printf("%s, tests run: %d\n", fail_msg ? "FAIL" : "PASS", s_num_tests);
  return fail_msg == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
