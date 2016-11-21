/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp8266/user/esp_features.h"

#ifndef MIOT_ENABLE_HEAP_LOG
#define MIOT_ENABLE_HEAP_LOG 0
#endif

#if MIOT_ENABLE_HEAP_LOG

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if MIOT_ENABLE_JS
#include "v7/v7.h"
#endif

#include "esp_mem_layout.h"

/*
 * global flag that is needed for heap trace: we shouldn't send anything to
 * uart until it is initialized
 */
int uart_initialized = 0;

extern int cs_heap_shim;

/* Defined in linker script. */
extern unsigned int _heap_start;

extern void *__real_pvPortRealloc(void *pv, size_t size, const char *file,
                                  int line);
extern void *__real_pvPortMalloc(size_t xWantedSize, const char *file,
                                 int line);
extern void *__real_pvPortCalloc(size_t num, size_t xWantedSize,
                                 const char *file, int line);
extern void *__real_pvPortZalloc(size_t size, const char *file, int line);
extern void __real_vPortFree(void *pv, const char *file, int line);

extern void miot_wdt_feed(void);

void print_call_trace();

/*
 * Maximum amount of calls to malloc/free and other friends before UART is
 * initialized. At the moment of writing this, there are 44 calls. Let it be
 * 50. (if actual amount exceeds, we'll abort)
 */
#define LOG_ITEMS_CNT 120

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
  fprintf(stderr, "hl{m,%u,%d,", (unsigned int) size, shim);
}

NOINSTR
static void echo_log_zalloc_req(size_t size, int shim) {
  fprintf(stderr, "hl{z,%u,%d,", (unsigned int) size, shim);
}

NOINSTR
static void echo_log_calloc_req(size_t size, int shim) {
  fprintf(stderr, "hl{c,%u,%d,", (unsigned int) size, shim);
}

NOINSTR
static void echo_log_realloc_req(size_t size, int shim, void *old_ptr) {
  fprintf(stderr, "hl{r,%u,%d,%x,", (unsigned int) size, shim,
          (unsigned int) old_ptr);
}

NOINSTR
static void echo_log_alloc_res(void *ptr) {
#if MIOT_ENABLE_CALL_TRACE
  fprintf(stderr, "%x} ", (unsigned int) ptr);
  if (plog == NULL) {
    print_call_trace();
  } else {
    fprintf(stderr, "\n");
  }
#else
  fprintf(stderr, "%x}\n", (unsigned int) ptr);
#endif
}

NOINSTR
static void echo_log_free(void *ptr, int shim) {
#if MIOT_ENABLE_CALL_TRACE
  fprintf(stderr, "hl{f,%x,%d} ", (unsigned int) ptr, shim);
  if (plog == NULL) {
    print_call_trace();
  } else {
    fprintf(stderr, "\n");
  }
#else
  fprintf(stderr, "hl{f,%x,%d}\n", (unsigned int) ptr, shim);
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
    plog = __real_pvPortCalloc(1, sizeof(*plog), NULL, 0);
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
      fprintf(stderr, "struct log_item::size width is too small\n");
      abort();
    } else if (plog->log_items_cnt_overflow > 0) {
      fprintf(stderr, "LOG_ITEMS_CNT is too low (need at least %u)\n",
              LOG_ITEMS_CNT + plog->log_items_cnt_overflow);
      abort();
    } else if (plog->ptr_doesnt_fit) {
      fprintf(stderr, "ptr is not suitable for PTR_INFLATE\n");
      abort();
    }

    fprintf(stderr, "\nhlog_param:{\"heap_start\":%u, \"heap_end\":%u}\n",
            (unsigned int) (&_heap_start), (unsigned int) ESP_DRAM0_END);

    /* fprintf above may have use malloc and already flushed the log.
     * Ideally, we should not be touching heap while flushing the log,
     * but this will do as a workaround.
     * As a reasult, the header is printed twice. */
    if (plog == NULL) return;

    for (i = 0; i < plog->items_cnt; i++) {
      echo_log_item(&plog->items[i]);
    }

    __real_vPortFree(plog, NULL, 0);
    plog = NULL;
    fprintf(stderr, "--- uart initialized ---\n");
    miot_wdt_feed();
  }
}

/*
 * Wrappers for heap functions
 */

void *__wrap_pvPortRealloc(void *pv, size_t size, const char *file, int line) {
  void *ret = NULL;
  if (uart_initialized) {
    flush_log_items();
    echo_log_realloc_req(size, cs_heap_shim, pv);
  }
  ret = __real_pvPortRealloc(pv, size, file, line);
  if (uart_initialized) {
    miot_wdt_feed();
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

void *__wrap_pvPortMalloc(size_t xWantedSize, const char *file, int line) {
  void *ret = NULL;
  if (uart_initialized) {
    flush_log_items();
    echo_log_malloc_req(xWantedSize, cs_heap_shim);
  }
  ret = __real_pvPortMalloc(xWantedSize, file, line);
  if (uart_initialized) {
    miot_wdt_feed();
    echo_log_alloc_res(ret);
  } else {
    add_log_item(ITEM_TYPE_MALLOC, ret, xWantedSize, cs_heap_shim);
  }

  cs_heap_shim = 0;
  return ret;
}

void *__wrap_pvPortZalloc(size_t xWantedSize, const char *file, int line) {
  void *ret = NULL;
  if (uart_initialized) {
    flush_log_items();
    echo_log_malloc_req(xWantedSize, cs_heap_shim);
  }
  ret = __real_pvPortZalloc(xWantedSize, file, line);
  if (uart_initialized) {
    miot_wdt_feed();
    echo_log_alloc_res(ret);
  } else {
    add_log_item(ITEM_TYPE_ZALLOC, ret, xWantedSize, cs_heap_shim);
  }
  cs_heap_shim = 0;
  return ret;
}

void *__wrap_pvPortCalloc(size_t num, size_t xWantedSize, const char *file,
                          int line) {
  void *ret = NULL;
  if (uart_initialized) {
    flush_log_items();
    echo_log_calloc_req(xWantedSize, cs_heap_shim);
  }
  ret = __real_pvPortCalloc(num, xWantedSize, file, line);

  if (uart_initialized) {
    miot_wdt_feed();
    echo_log_alloc_res(ret);
  } else {
    add_log_item(ITEM_TYPE_CALLOC, ret, xWantedSize, cs_heap_shim);
  }

  cs_heap_shim = 0;
  return ret;
}

void __wrap_vPortFree(void *pv, const char *file, int line) {
  if (uart_initialized) {
    flush_log_items();
    miot_wdt_feed();
    echo_log_free(pv, cs_heap_shim);
  } else {
    add_log_item(ITEM_TYPE_FREE, pv, 0, cs_heap_shim);
  }
  __real_vPortFree(pv, file, line);
  cs_heap_shim = 0;
}

#endif /* ESP_ENABLE_HEAP_LOG */
