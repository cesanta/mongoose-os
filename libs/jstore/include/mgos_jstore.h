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

#ifndef CS_MOS_LIBS_JSTORE_SRC_MGOS_JSTORE_H_
#define CS_MOS_LIBS_JSTORE_SRC_MGOS_JSTORE_H_

#include <stdbool.h>
#include <stdint.h>

#include "common/mg_str.h"
#include "mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mgos_jstore;

/*
 * Ownership of the string data
 */
enum mgos_jstore_ownership {
  /* Data is copied, copy is retained by jstore and freed when appropriate */
  MGOS_JSTORE_OWN_COPY,
  /* Data is NOT copied, but is retained by jstore and freed when appropriate */
  MGOS_JSTORE_OWN_RETAIN,
  /*
   * Data is NOT copied, jstore uses data and never frees it. Suitable for e.g.
   * constant data.
   */
  MGOS_JSTORE_OWN_FOREIGN,

  /*
   * Should be used when the ownership parameter is irrelevant: e.g. the
   * ownership of id when the id was not provided
   */
  MGOS_JSTORE_OWN_INVALID,
};

/*
 * Opaque item handle, can be used for the efficient editing/removing an item.
 */
typedef uintptr_t mgos_jstore_item_hnd_t;

/*
 * Type of the reference: by struct mg_str id, int index, or
 * mgos_jstore_item_hnd_t hnd.
 *
 * Typically users don't need to use those manually; instead, use macros below:
 * `MGOS_JSTORE_REF_BY_...()`.
 */
enum mgos_jstore_ref_type {
  MGOS_JSTORE_REF_TYPE_INVALID,
  MGOS_JSTORE_REF_TYPE_BY_ID,
  MGOS_JSTORE_REF_TYPE_BY_INDEX,
  MGOS_JSTORE_REF_TYPE_BY_HND,
};

/*
 * Reference to a particular item in the store. Don't use that structure
 * directly, use macros below: `MGOS_JSTORE_REF_BY_ID()`, etc.
 */
struct mgos_jstore_ref {
  enum mgos_jstore_ref_type type;
  union {
    struct mg_str id;
    int index;
    mgos_jstore_item_hnd_t hnd;
  } data;
};

/*
 * Constructs reference to an item by the given struct mg_str id.
 */
#define MGOS_JSTORE_REF_BY_ID(x)                               \
  ((struct mgos_jstore_ref){                                   \
      .type = MGOS_JSTORE_REF_TYPE_BY_ID, .data = {.id = (x)}, \
  })

/*
 * Constructs reference to an item by the given int index.
 */
#define MGOS_JSTORE_REF_BY_INDEX(x)                                  \
  ((struct mgos_jstore_ref){                                         \
      .type = MGOS_JSTORE_REF_TYPE_BY_INDEX, .data = {.index = (x)}, \
  })

/*
 * Constructs reference to an item by the given opaque handler
 * mgos_jstore_item_hnd_t hnd.
 */
#define MGOS_JSTORE_REF_BY_HND(x)                                \
  ((struct mgos_jstore_ref){                                     \
      .type = MGOS_JSTORE_REF_TYPE_BY_HND, .data = {.hnd = (x)}, \
  })

/*
 * Constructs invalid reference to an item
 */
#define MGOS_JSTORE_REF_INVALID             \
  ((struct mgos_jstore_ref){                \
      .type = MGOS_JSTORE_REF_TYPE_INVALID, \
  })

/*
 * Create jstore from the JSON file `json_path`. If file does not exist or
 * is empty, it's not an error and will just result in an empty jstore.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
struct mgos_jstore *mgos_jstore_create(const char *json_path, char **perr);

/*
 * Callback for `mgos_jstore_iterate`, called for each item in the jstore.
 * `idx` is a zero-based index of the item, `hnd` is an opaque item's
 * handle, can be used for editing or removing it without having to look for
 * the item by the id.
 *
 * The callback should return true to continue iteration, or false to stop.
 */
typedef bool (*mgos_jstore_cb)(struct mgos_jstore *store, int idx,
                               mgos_jstore_item_hnd_t hnd,
                               const struct mg_str *id,
                               const struct mg_str *data, void *userdata);

/*
 * Call provided callback for each item in the store; see `mgos_jstore_cb` for
 * details.
 *
 * Returns false if the callback has returned false at least once. Returns true
 * if callback never returned false.
 */
bool mgos_jstore_iterate(struct mgos_jstore *store, mgos_jstore_cb cb,
                         void *userdata);

/*
 * Add a new item to the store. If `id` contains some data (`id.p` is not NULL),
 * the provided id will be used; otherwise, the id will be randomly generated.
 * In any case, the actual id is returned; the caller should NOT free it,
 * and it remains valid until the store item is freed.
 *
 * Data should be a valid JSON string. Examples of valid data:
 *
 * - Array: `"[\"foo", "bar\"]"`
 * - String: `"\"foo bar\""` (with explicit quotes)
 *
 * Plain `"foo bar"` would be an invalid data.
 *
 * Ownership of `id` and `data` is determined by `id_own` and `data_own`,
 * see `enum mgos_jstore_ownership`.
 *
 * If `phnd` is not NULL, the new item's handle is written there.
 * If `pindex` is not NULL, the new item's index is written there.
 *
 * Returns the id of a new item. In case of an error, that id will be empty.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
struct mg_str mgos_jstore_item_add(struct mgos_jstore *store, struct mg_str id,
                                   struct mg_str data,
                                   enum mgos_jstore_ownership id_own,
                                   enum mgos_jstore_ownership data_own,
                                   mgos_jstore_item_hnd_t *phnd, int *pindex,
                                   char **perr);

/*
 * Edit item by the reference (see `MGOS_JSTORE_REF_BY_...()` macros above)
 *
 * Returns true in case of success, false otherwise.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
bool mgos_jstore_item_edit(struct mgos_jstore *store,
                           const struct mgos_jstore_ref ref, struct mg_str data,
                           enum mgos_jstore_ownership data_own, char **perr);

/*
 * Remove item by the reference (see `MGOS_JSTORE_REF_BY_...()` macros above)
 *
 * Returns true in case of success, false otherwise.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
bool mgos_jstore_item_remove(struct mgos_jstore *store,
                             const struct mgos_jstore_ref ref, char **perr);

/*
 * Get item details by the given reference (see `MGOS_JSTORE_REF_BY_...()`
 * macros above). All output pointers (`id`, `data`, `phnd`, `pindex`) are
 * allowed to be NULL.
 *
 * Returns true in case of success, false otherwise.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
bool mgos_jstore_item_get(struct mgos_jstore *store,
                          const struct mgos_jstore_ref ref, struct mg_str *id,
                          struct mg_str *data, mgos_jstore_item_hnd_t *phnd,
                          int *pindex, char **perr);

/*
 * Save jstore to the JSON file `json_path`.
 *
 * Returns true in case of success, false otherwise.
 *
 * If `perr` is not NULL, the error message will be written there (or NULL
 * in case of success). The caller should free the error message.
 */
bool mgos_jstore_save(struct mgos_jstore *store, const char *json_path,
                      char **perr);

/*
 * Get number of items in a jstore.
 */
int mgos_jstore_items_cnt(struct mgos_jstore *store);

/*
 * Free memory occupied by jstore and all its items.
 */
void mgos_jstore_free(struct mgos_jstore *store);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_JSTORE_SRC_MGOS_JSTORE_H_ */
