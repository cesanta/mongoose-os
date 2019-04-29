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

#include "esp_features.h"

#ifndef MGOS_ENABLE_HEAP_LOG
#define MGOS_ENABLE_HEAP_LOG 0
#endif

#if MGOS_ENABLE_HEAP_LOG

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "umm_malloc_cfg.h"

#include "mgos_debug.h"
#include "mgos_uart.h"

#include "esp_exc.h"

/*
 * global flag that is needed for heap trace: we shouldn't send anything to
 * uart until it is initialized
 */
extern bool uart_initialized;

extern int cs_heap_shim;

/* Defined in linker script. */
extern unsigned int _heap_start;

extern void *__real_umm_malloc(size_t size);
extern void *__real_umm_calloc(size_t num, size_t size);
extern void *__real_umm_realloc(void *ptr, size_t size);
extern void __real_umm_free(void *ptr);

extern void mgos_wdt_feed(void);

void print_call_trace();

/*
 * Maximum amount of calls to malloc/free and other friends before UART is
 * initialized. At the moment of writing this, there are 44 calls. Let it be
 * 50. (if actual amount exceeds, we'll abort)
 */
#define LOG_ITEMS_CNT 200

/*
 * Special value to be saved into `struct log_item::ptr`, meaning NULL pointer
 * (we can't use real NULL here because it would mean `0x3ffc0000`, see
 * `PTR_DEFLATE()` / `PTR_INFLATE()`)
 */
#define NULL_FLAG 0x01

/*
 * Takes a pointer and yields a truncated value to be saved into `struct
 * log_item::ptr`
 *
 * See `PTR_INFLATE()`
 */
#define PTR_DEFLATE(ptr) \
  ((ptr) ? ((unsigned int) (ptr) &0x0003ffff) : NULL_FLAG)

/*
 * Takes a truncated pointer value stored into `struct log_item::ptr`, and
 * yields a real pointer
 *
 * See `PTR_DEFLATE()`
 */
#define PTR_INFLATE(v) \
  (((v) != NULL_FLAG) ? ((void *) (0x3ffc0000 | (v))) : NULL)

typedef uint8_t item_type_t;
enum item_type {
  ITEM_TYPE_MALLOC,
  ITEM_TYPE_ZALLOC,
  ITEM_TYPE_CALLOC,
  ITEM_TYPE_FREE,
};

/*
 * malloc/free log item. Does not support realloc (since reallocs don't happen
 * with uninitialized uart)
 */
struct log_item {
  /* 16 less-significant bits of pointer: no need to store the rest */
  unsigned ptr : 18;
  unsigned size : 11;
  enum item_type type : 2;
  unsigned our_shim : 1;
};

/* this structure is allocated from heap until uart is initialized */
struct log {
  struct log_item items[LOG_ITEMS_CNT];
  int items_cnt;

  unsigned ptr_doesnt_fit : 1;
  unsigned item_size_small : 1;
  unsigned log_items_cnt_overflow : 30;
};

static struct log *plog = NULL;

/*
 * functions that echo heap log
 */

NOINSTR
static void echo_log_malloc_req(size_t size, int shim) {
  esp_exc_printf("hl{m,%u,%d,", (unsigned int) size, shim);
}

NOINSTR
static void echo_log_zalloc_req(size_t size, int shim) {
  esp_exc_printf("hl{z,%u,%d,", (unsigned int) size, shim);
}

NOINSTR
static void echo_log_calloc_req(size_t size, int shim) {
  esp_exc_printf("hl{c,%u,%d,", (unsigned int) size, shim);
}

NOINSTR
static void echo_log_realloc_req(size_t size, int shim, void *old_ptr) {
  esp_exc_printf("hl{r,%u,%d,%x,", (unsigned int) size, shim,
                 (unsigned int) old_ptr);
}

NOINSTR
static void echo_log_alloc_res(void *ptr) {
#if MGOS_ENABLE_CALL_TRACE
  esp_exc_printf("%x} ", (unsigned int) ptr);
  if (plog == NULL) {
    print_call_trace();
  } else {
    esp_exc_printf("\n");
  }
#else
  esp_exc_printf("%x}\n", (unsigned int) ptr);
#endif
}

NOINSTR
static void echo_log_free(void *ptr, int shim) {
#if MGOS_ENABLE_CALL_TRACE
  esp_exc_printf("hl{f,%x,%d} ", (unsigned int) ptr, shim);
  if (plog == NULL) {
    print_call_trace();
  } else {
    esp_exc_printf("\n");
  }
#else
  esp_exc_printf("hl{f,%x,%d}\n", (unsigned int) ptr, shim);
#endif
}

/*
 * add one item to log. Used before uart is initialized
 */
static void add_log_item(enum item_type type, void *ptr, size_t size,
                         int shim) {
  uint8_t plog_allocated = 0;
  struct log_item item = {0};
  item.ptr = PTR_DEFLATE(ptr);
  item.size = size;
  item.type = type;
  item.our_shim = shim;

  if (PTR_INFLATE(item.ptr) != ptr) {
    /* NOTE: we can't printf here, since UART is not yet initialized */
    plog->ptr_doesnt_fit = 1;
  }

  if (size != item.size) {
    /* NOTE: we can't printf here, since UART is not yet initialized */
    plog->item_size_small = 1;
  }

  if (plog == NULL) {
    plog = __real_umm_calloc(1, sizeof(*plog));
    plog_allocated = 1;
  }

  if (plog->items_cnt >= LOG_ITEMS_CNT) {
    /* NOTE: we can't printf here, since UART is not yet initialized */
    plog->log_items_cnt_overflow++;
  }

  plog->items[plog->items_cnt] = item;
  plog->items_cnt++;

  /* we we've just allocated `plog` buffer, log this allocation as well */
  if (plog_allocated) {
    add_log_item(ITEM_TYPE_CALLOC, plog, sizeof(*plog), 1);
  }
}

/*
 * echo one log item. Used before uart is initialized
 */
NOINSTR
static void echo_log_item(struct log_item *item) {
  switch (item->type) {
    case ITEM_TYPE_MALLOC:
      echo_log_malloc_req((unsigned int) item->size, item->our_shim);
      echo_log_alloc_res(PTR_INFLATE((unsigned int) item->ptr));
      break;
    case ITEM_TYPE_ZALLOC:
      echo_log_zalloc_req((unsigned int) item->size, item->our_shim);
      echo_log_alloc_res(PTR_INFLATE((unsigned int) item->ptr));
      break;
    case ITEM_TYPE_CALLOC:
      echo_log_calloc_req((unsigned int) item->size, item->our_shim);
      echo_log_alloc_res(PTR_INFLATE((unsigned int) item->ptr));
      break;
    case ITEM_TYPE_FREE:
      echo_log_free(PTR_INFLATE((unsigned int) item->ptr), item->our_shim);
      break;
    default:
      abort();
      break;
  }
}

/*
 * if there are some pending log items, echo all of them, and then free log
 * structure
 */
static void flush_log_items(void) {
  if (plog != NULL) {
    int i;

    /* before we flush, we should also log that we freed `plog` buffer */
    add_log_item(ITEM_TYPE_FREE, plog, 0, 1);

    if (plog->item_size_small) {
      esp_exc_printf("struct log_item::size width is too small\n");
      abort();
    } else if (plog->log_items_cnt_overflow > 0) {
      esp_exc_printf("LOG_ITEMS_CNT is too low (need at least %u)\n",
                     LOG_ITEMS_CNT + plog->log_items_cnt_overflow);
      abort();
    } else if (plog->ptr_doesnt_fit) {
      esp_exc_printf("ptr is not suitable for PTR_INFLATE\n");
      abort();
    }

    esp_exc_printf("\nhlog_param:{\"heap_start\":%u, \"heap_end\":%u}\n",
                   (unsigned int) UMM_MALLOC_CFG__HEAP_ADDR,
                   (unsigned int) UMM_MALLOC_CFG__HEAP_END);

    /* fprintf above may have use malloc and already flushed the log.
     * Ideally, we should not be touching heap while flushing the log,
     * but this will do as a workaround.
     * As a reasult, the header is printed twice. */
    if (plog == NULL) return;

    for (i = 0; i < plog->items_cnt; i++) {
      echo_log_item(&plog->items[i]);
    }

    __real_umm_free(plog);
    plog = NULL;
    esp_exc_printf("--- uart initialized ---\n");
    mgos_wdt_feed();
  }
}

/*
 * Wrappers for heap functions
 */

void *__wrap_umm_realloc(void *ptr, size_t size) {
  void *ret = NULL;
  if (uart_initialized) {
    flush_log_items();
    echo_log_realloc_req(size, cs_heap_shim, ptr);
  }
  ret = __real_umm_realloc(ptr, size);
  if (uart_initialized) {
    mgos_wdt_feed();
    echo_log_alloc_res(ret);
  } else {
    /*
     * reallocs don't happen before uart is initialized, and our pre-uart
     * logging facility doesn't support reallocs since it needs for additional
     * pointer.
     */
    abort();
  }
  cs_heap_shim = 0;
  return ret;
}

void *__wrap_umm_malloc(size_t size) {
  void *ret = NULL;
  if (uart_initialized) {
    flush_log_items();
    echo_log_malloc_req(size, cs_heap_shim);
  }
  ret = __real_umm_malloc(size);
  if (uart_initialized) {
    mgos_wdt_feed();
    echo_log_alloc_res(ret);
  } else {
    add_log_item(ITEM_TYPE_MALLOC, ret, size, cs_heap_shim);
  }

  cs_heap_shim = 0;
  return ret;
}

void *__wrap_umm_calloc(size_t num, size_t size) {
  void *ret = NULL;
  if (uart_initialized) {
    flush_log_items();
    echo_log_calloc_req(num * size, cs_heap_shim);
  }
  ret = __real_umm_calloc(num, size);

  if (uart_initialized) {
    mgos_wdt_feed();
    echo_log_alloc_res(ret);
  } else {
    add_log_item(ITEM_TYPE_CALLOC, ret, size, cs_heap_shim);
  }

  cs_heap_shim = 0;
  return ret;
}

void __wrap_umm_free(void *ptr) {
  if (uart_initialized) {
    flush_log_items();
    mgos_wdt_feed();
    echo_log_free(ptr, cs_heap_shim);
  } else {
    add_log_item(ITEM_TYPE_FREE, ptr, 0, cs_heap_shim);
  }
  __real_umm_free(ptr);
  cs_heap_shim = 0;
}

#endif /* ESP_ENABLE_HEAP_LOG */
