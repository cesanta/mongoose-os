
#include <stdio.h>
#include <string.h>

#include "mgos_jstore.h"
#include "common/cs_file.h"

static int s_num_tests = 0;

#define STRINGIFY_MG_STR(a) \
  (mg_mk_str_n((STRINGIFY(a)) + 1, strlen(STRINGIFY(a)) - 2))
#define STRINGIFY(a) STRINGIFY2(a)
#define STRINGIFY2(a) #a

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

#define HOURS(x) ((x) *60 * 60)

struct jstore_expected_item {
  struct mg_str id;
  struct mg_str data;
  bool equal;
};

struct jstore_expected {
  int items_cnt;
  struct jstore_expected_item *items;

  int next_idx;
};

static bool item_compare(struct mgos_jstore *store, int idx,
                         mgos_jstore_item_hnd_t hnd, const struct mg_str *id,
                         const struct mg_str *data, void *userdata) {
  struct jstore_expected *expected = (struct jstore_expected *) userdata;

  expected->items[idx].equal = true;

  if (expected->items[idx].id.p != NULL) {
    if (expected->items[idx].equal) {
      expected->items[idx].equal = id->len == expected->items[idx].id.len;
    }
    if (expected->items[idx].equal) {
      expected->items[idx].equal = strncmp(expected->items[idx].id.p, id->p,
                                           expected->items[idx].id.len) == 0;
    }
  } else {
    /* Don't check id */
  }

  if (expected->items[idx].equal) {
    expected->items[idx].equal = data->len == expected->items[idx].data.len;
  }
  if (expected->items[idx].equal) {
    expected->items[idx].equal = strncmp(expected->items[idx].data.p, data->p,
                                         expected->items[idx].data.len) == 0;
  }

  if (!expected->items[idx].equal) {
    printf(
        "Item #%d: expected id: '%.*s', actual id: '%.*s'; expected data: "
        "'%.*s', actual data: '%.*s'\n",
        idx, expected->items[idx].id.len, expected->items[idx].id.p, id->len,
        id->p, expected->items[idx].data.len, expected->items[idx].data.p,
        data->len, data->p);
  }

  return true;
}

static bool item_remove_7_8(struct mgos_jstore *store, int idx,
                            mgos_jstore_item_hnd_t hnd, const struct mg_str *id,
                            const struct mg_str *data, void *userdata) {
  if (idx == 7 || idx == 8) {
    char *err = NULL;
    mgos_jstore_item_remove(store, MGOS_JSTORE_REF_BY_HND(hnd), &err);
    if (err != NULL) {
      printf("error: %s\n", err);
      abort();
    }
  }

  if (idx == 8) {
    return false;
  }

  if (idx > 8) {
    printf("got index=%d, but should have stopped earlier", idx);
    abort();
  }

  return true;
}

static const char *s_test1(void) {
  int i;
  char *err = NULL;
  struct mgos_jstore *store = mgos_jstore_create("test1.json", &err);
  if (err != NULL) {
    printf("error: %s\n", err);
  }
  ASSERT(err == NULL);

  struct jstore_expected expected;
  memset(&expected, 0, sizeof(expected));

  /* Check initial test1.json contents */

  expected.items = calloc(100, sizeof(struct jstore_expected_item));

  /* clang-format off */
  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
    .id = mg_mk_str("0"),
    .data = STRINGIFY_MG_STR(({"at": "@sunrise", "action": "slide.stub_sunrise", "payload": {"foo": 123}}))
  };

  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
    .id = mg_mk_str("1"),
    .data = STRINGIFY_MG_STR(({"at": "@sunset", "action": "slide.stub_sunset"}))
  };

  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
    .id = mg_mk_str("2"),
    .data = mg_mk_str("[\"foo\", \"bar\", 123, true, {\"qqq\": []}]")
  };

  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
    .id = mg_mk_str("3"),
    .data = mg_mk_str("true")
  };

  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
    .id = mg_mk_str("4"),
    .data = STRINGIFY_MG_STR((123))
  };

  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
    .id = mg_mk_str("5"),
    .data = mg_mk_str("false")
  };

  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
    .id = mg_mk_str("6"),
    .data = STRINGIFY_MG_STR(("sdf"))
  };

  /* clang-format on */

  ASSERT(mgos_jstore_iterate(store, item_compare, &expected));
  for (i = 0; i < expected.items_cnt; i++) {
    ASSERT(expected.items[i].equal);
  }

  /* Add items with an explicit id */

  ASSERT(mgos_jstore_item_add(store, mg_mk_str("custom_id"),
                              mg_mk_str("\"new data 1\""),
                              MGOS_JSTORE_OWN_FOREIGN, MGOS_JSTORE_OWN_FOREIGN,
                              NULL, NULL, NULL).p != NULL);

  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
      .id = mg_mk_str("custom_id"), .data = mg_mk_str("\"new data 1\"")};

  /* Add items with an a random id (which is returned via `id` pointer) */

  struct mg_str id = mgos_jstore_item_add(
      store, mg_mk_str(NULL), mg_mk_str("\"new data 2\""),
      MGOS_JSTORE_OWN_FOREIGN, MGOS_JSTORE_OWN_FOREIGN, NULL, NULL, NULL);

  ASSERT(id.p != NULL);
  ASSERT(id.len != 0);

  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
      .id = id, .data = mg_mk_str("\"new data 2\"")};

  /* Add items with an a random id, which we ignore */

  int index;

  ASSERT(mgos_jstore_item_add(store, mg_mk_str(NULL),
                              mg_mk_str("\"new data 3\""), MGOS_JSTORE_OWN_COPY,
                              MGOS_JSTORE_OWN_COPY, NULL, &index,
                              NULL).p != NULL);

  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
      .id = mg_mk_str(NULL), .data = mg_mk_str("\"new data 3\"")};

  /* Check new contents */

  ASSERT(mgos_jstore_iterate(store, item_compare, &expected));
  for (i = 0; i < expected.items_cnt; i++) {
    ASSERT(expected.items[i].equal);
  }

  /* Edit an item by index */

  ASSERT(mgos_jstore_item_edit(store, MGOS_JSTORE_REF_BY_INDEX(index),
                               mg_mk_str("\"new data 3, updated\""),
                               MGOS_JSTORE_OWN_FOREIGN, NULL));
  expected.items[expected.items_cnt - 1].data =
      mg_mk_str("\"new data 3, updated\"");

  ASSERT(mgos_jstore_item_edit(store, MGOS_JSTORE_REF_BY_INDEX(3),
                               mg_mk_str("\"new third item\""),
                               MGOS_JSTORE_OWN_FOREIGN, NULL));
  expected.items[3].data = mg_mk_str("\"new third item\"");

  /* Check new contents */

  ASSERT(mgos_jstore_iterate(store, item_compare, &expected));
  for (i = 0; i < expected.items_cnt; i++) {
    ASSERT(expected.items[i].equal);
  }

  /* Try to edit non-existing item */

  mgos_jstore_item_edit(store, MGOS_JSTORE_REF_BY_ID(mg_mk_str("sdf")),
                        mg_mk_str("sdfsdf sf"), MGOS_JSTORE_OWN_FOREIGN, &err);
  ASSERT(strcmp(err, "item not found") == 0);
  free(err);
  err = NULL;

  /* Check contents (should stay the same) */

  ASSERT(mgos_jstore_iterate(store, item_compare, &expected));
  for (i = 0; i < expected.items_cnt; i++) {
    ASSERT(expected.items[i].equal);
  }

  /* Add one more item and try to edit it by the item's handle */

  mgos_jstore_item_hnd_t hnd;

  ASSERT(mgos_jstore_item_add(store, mg_mk_str("hey_id_4"),
                              mg_mk_str("\"new data 4\""), MGOS_JSTORE_OWN_COPY,
                              MGOS_JSTORE_OWN_COPY, &hnd, NULL,
                              NULL).p != NULL);

  expected.items[expected.items_cnt++] = (struct jstore_expected_item){
      .id = mg_mk_str(NULL), .data = mg_mk_str("\"new data 4\"")};

  ASSERT(mgos_jstore_item_edit(store, MGOS_JSTORE_REF_BY_HND(hnd),
                               mg_mk_str("\"new data 4, updated\""),
                               MGOS_JSTORE_OWN_FOREIGN, NULL));
  expected.items[expected.items_cnt - 1].data =
      mg_mk_str("\"new data 4, updated\"");

  /* Remove "5" */

  ASSERT(mgos_jstore_item_remove(store, MGOS_JSTORE_REF_BY_ID(mg_mk_str("5")),
                                 NULL));

  for (i = 5; i < 10; i++) {
    expected.items[i] = expected.items[i + 1];
  }
  expected.items_cnt--;

  /* Check contents */

  ASSERT(mgos_jstore_iterate(store, item_compare, &expected));
  for (i = 0; i < expected.items_cnt; i++) {
    ASSERT(expected.items[i].equal);
  }

  /* Remove items with random ids, so that we could compare file contents */

  ASSERT(!mgos_jstore_iterate(store, item_remove_7_8, NULL));

  /* Save jstore in a file and compare with expected contents */

  ASSERT(mgos_jstore_save(store, "test1_updated.json", NULL));
  {
    size_t len;
    char *data = cs_read_file("test1_updated.json", &len);

    size_t len_expected;
    char *data_expected =
        cs_read_file("test1_updated_expected.json", &len_expected);

    ASSERT(len == len_expected);
    ASSERT(strcmp(data, data_expected) == 0);
  }

  /* Test getting item details */

  struct mg_str data;
  hnd = 0;
  ASSERT(mgos_jstore_item_get(store, MGOS_JSTORE_REF_BY_INDEX(4), &id, &data,
                              &hnd, &index, NULL));
  ASSERT(mg_vcmp(&id, "4") == 0);
  ASSERT(mg_vcmp(&data, "123") == 0);
  ASSERT(hnd != 0);
  ASSERT(index == 4);

  /* Try to get the same item, but by its id, and compare handlers: should be
   * the same */

  mgos_jstore_item_hnd_t hnd2 = 0;
  ASSERT(mgos_jstore_item_get(store, MGOS_JSTORE_REF_BY_ID(mg_mk_str("4")),
                              NULL, NULL, &hnd2, NULL, NULL));

  ASSERT(hnd == hnd2);

  /* Try getting an item with all output params set to NULL: might be used to
   * check if item exists */

  ASSERT(mgos_jstore_item_get(store, MGOS_JSTORE_REF_BY_ID(mg_mk_str("6")),
                              NULL, NULL, NULL, NULL, NULL));

  /* Try getting non-existing item: should return an error */

  ASSERT(!mgos_jstore_item_get(store, MGOS_JSTORE_REF_BY_ID(mg_mk_str("5")),
                               NULL, NULL, NULL, NULL, NULL));

  err = NULL;

  ASSERT(!mgos_jstore_item_get(store, MGOS_JSTORE_REF_BY_ID(mg_mk_str("5")),
                               NULL, NULL, NULL, NULL, &err));
  ASSERT(err != NULL);
  free(err);

  mgos_jstore_free(store);
  store = NULL;

  return NULL;
}

static const char *s_run_all_tests(void) {
  RUN_TEST(s_test1);
  return NULL;
}

int main(void) {
  const char *fail_msg = s_run_all_tests();
  printf("%s, tests run: %d\n", fail_msg ? "FAIL" : "PASS", s_num_tests);
  return fail_msg == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
