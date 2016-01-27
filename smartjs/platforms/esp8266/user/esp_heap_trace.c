/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#if defined(ESP_ENABLE_HEAP_LOG)

#include <stdio.h>
#include <stdlib.h>

#include "v7/v7.h"
#include "esp_mem_layout.h"

extern int uart_initialized;
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

extern void sj_wdt_feed(void);

/*
 * Maximum amount of calls to malloc/free and other friends before UART is
 * initialized. At the moment of writing this, there are 44 calls. Let it be
 * 50. (if actual amount exceeds, we'll abort)
 */
#define LOG_ITEMS_CNT 50

#define MK_PTR(v) ((void *) (0x3ffc0000 | (v)))

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
  unsigned log_items_cnt_small : 1;
};

static struct log *plog = NULL;

/*
 * functions that echo heap log
 */

static void echo_log_malloc_req(size_t size, int shim) {
  printf("hl{m,%u,%d,", (unsigned int) size, shim);
}

static void echo_log_zalloc_req(size_t size, int shim) {
  printf("hl{z,%u,%d,", (unsigned int) size, shim);
}

static void echo_log_calloc_req(size_t size, int shim) {
  printf("hl{c,%u,%d,", (unsigned int) size, shim);
}

static void echo_log_realloc_req(size_t size, int shim, void *old_ptr) {
  printf("hl{r,%u,%d,%x,", (unsigned int) size, shim, (unsigned int) old_ptr);
}

static void echo_log_alloc_res(void *ptr) {
  printf("%x}\n", (unsigned int) ptr);
}

static void echo_log_free(void *ptr, int shim) {
  printf("hl{f,%x,%d}\n", (unsigned int) ptr, shim);
}

/*
 * add one item to log. Used before uart is initialized
 */
static void add_log_item(enum item_type type, void *ptr, size_t size,
                         int shim) {
  uint8_t plog_allocated = 0;
  struct log_item item = {0};
  item.ptr = (unsigned int) ptr & 0x3ffff;
  item.size = size;
  item.type = type;
  item.our_shim = shim;

  if (MK_PTR(item.ptr) != ptr) {
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
    plog->log_items_cnt_small = 1;
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
static void echo_log_item(struct log_item *item) {
  switch (item->type) {
    case ITEM_TYPE_MALLOC:
      echo_log_malloc_req((unsigned int) item->size, item->our_shim);
      echo_log_alloc_res(MK_PTR((unsigned int) item->ptr));
      break;
    case ITEM_TYPE_ZALLOC:
      echo_log_zalloc_req((unsigned int) item->size, item->our_shim);
      echo_log_alloc_res(MK_PTR((unsigned int) item->ptr));
      break;
    case ITEM_TYPE_CALLOC:
      echo_log_calloc_req((unsigned int) item->size, item->our_shim);
      echo_log_alloc_res(MK_PTR((unsigned int) item->ptr));
      break;
    case ITEM_TYPE_FREE:
      echo_log_free(MK_PTR((unsigned int) item->ptr), item->our_shim);
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
      printf("struct log_item::size width is too small\n");
      abort();
    } else if (plog->log_items_cnt_small) {
      printf("LOG_ITEMS_CNT is too low\n");
      abort();
    } else if (plog->ptr_doesnt_fit) {
      printf("ptr is not suitable for MK_PTR\n");
      abort();
    }

    printf("hlog_param:{\"heap_start\":0x%x, \"heap_end\":0x%x}\n",
           (unsigned int) (&_heap_start), (unsigned int) ESP_DRAM0_END);

    for (i = 0; i < plog->items_cnt; i++) {
      echo_log_item(&plog->items[i]);
    }

    __real_vPortFree(plog, NULL, 0);
    plog = NULL;
    printf("--- uart initialized ---\n");
    sj_wdt_feed();
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
    sj_wdt_feed();
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
    sj_wdt_feed();
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
    sj_wdt_feed();
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
    sj_wdt_feed();
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
    sj_wdt_feed();
    echo_log_free(pv, cs_heap_shim);
  } else {
    add_log_item(ITEM_TYPE_FREE, pv, 0, cs_heap_shim);
  }
  __real_vPortFree(pv, file, line);
  cs_heap_shim = 0;
}

#endif /* ESP_ENABLE_HEAP_LOG */
