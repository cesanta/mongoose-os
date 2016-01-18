/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "v7/src/internal.h"

#include <stdio.h>
#include <stdlib.h>

#include <ets_sys.h>

#include <v7.h>
#include <sj_i2c.h>

#include <cs_dbg.h>

#include "esp_gpio.h"
#include "esp_periph.h"
#include "esp_missing_includes.h"
#ifndef RTOS_SDK

#include <osapi.h>
#include <gpio.h>

#else

#include <gpio_register.h>
#include <pin_mux_register.h>
#include <eagle_soc.h>
#include <freertos/portmacro.h>

#endif /* RTOS_SDK */

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

#if defined(ESP_ENABLE_HEAP_TRACE)

/*
 * Maximum amount of calls to malloc/free and other friends before UART is
 * initialized. At the moment of writing this, there are 44 calls. Let it be
 * 50. (if actual amount exceeds, we'll abort)
 */
#define LOG_ITEMS_CNT 50

#define MK_PTR(v) ((void *) (0x3fff0000 | (v)))

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
  uint16_t ptr;
  unsigned size : 13;
  enum item_type type : 2;
  unsigned our_shim : 1;
};

/* this structure is allocated from heap until uart is initialized */
struct log {
  struct log_item items[LOG_ITEMS_CNT];
  int items_cnt;

  unsigned item_size_small : 1;
  unsigned log_items_cnt_small : 1;
};

static struct log *plog = NULL;

/*
 * functions that echo heap log
 */

static void echo_log_malloc(size_t size, void *ptr, int shim) {
  fprintf(stderr, "hl{m,%u,%x,%d}\n", (unsigned int) size, (unsigned int) ptr,
          shim);
}

static void echo_log_zalloc(size_t size, void *ptr, int shim) {
  fprintf(stderr, "hl{z,%u,%x,%d}\n", (unsigned int) size, (unsigned int) ptr,
          shim);
}

static void echo_log_calloc(size_t size, void *ptr, int shim) {
  fprintf(stderr, "hl{c,%u,%x,%d}\n", (unsigned int) size, (unsigned int) ptr,
          shim);
}

static void echo_log_realloc(size_t size, void *old_ptr, void *ptr, int shim) {
  fprintf(stderr, "hl{r,%u,%x,%x,%d}\n", (unsigned int) size,
          (unsigned int) old_ptr, (unsigned int) ptr, shim);
}

static void echo_log_free(void *ptr, int shim) {
  fprintf(stderr, "hl{f,%x,%d}\n", (unsigned int) ptr, shim);
}

/*
 * add one item to log. Used before uart is initialized
 */
static void add_log_item(enum item_type type, void *ptr, size_t size,
                         int shim) {
  uint8_t plog_allocated = 0;
  struct log_item item = {0};
  item.ptr = (unsigned int) ptr & 0xffff;
  item.size = size;
  item.type = type;
  item.our_shim = shim;

  if (size != item.size) {
    /* NOTE: we can't fprintf here, since UART is not yet initialized */
    plog->item_size_small = 1;
  }

  if (plog == NULL) {
    plog = __real_pvPortCalloc(1, sizeof(*plog), NULL, 0);
    plog_allocated = 1;
  }

  if (plog->items_cnt >= LOG_ITEMS_CNT) {
    /* NOTE: we can't fprintf here, since UART is not yet initialized */
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
      echo_log_malloc((unsigned int) item->size,
                      MK_PTR((unsigned int) item->ptr), item->our_shim);
      break;
    case ITEM_TYPE_ZALLOC:
      echo_log_zalloc((unsigned int) item->size,
                      MK_PTR((unsigned int) item->ptr), item->our_shim);
      break;
    case ITEM_TYPE_CALLOC:
      echo_log_calloc((unsigned int) item->size,
                      MK_PTR((unsigned int) item->ptr), item->our_shim);
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
      fprintf(stderr, "struct log_item::size width is too small\n");
      abort();
    } else if (plog->log_items_cnt_small) {
      fprintf(stderr, "LOG_ITEMS_CNT is too low\n");
      abort();
    }

    /* TODO(dfrank) : heap size */
    fprintf(stderr, "hlog_param:{\"heap_start\":0x%x, \"heap_end\":0x%x}\n",
            (unsigned int) &_heap_start, (unsigned int) 0x3fff7fff);

    for (i = 0; i < plog->items_cnt; i++) {
      echo_log_item(&plog->items[i]);
    }

    __real_vPortFree(plog, NULL, 0);
    plog = NULL;
    fprintf(stderr, "--- uart initialized ---\n");
    sj_wdt_feed();
  }
}

/*
 * Wrappers for heap functions
 */

void *__wrap_pvPortRealloc(void *pv, size_t size, const char *file, int line) {
  void *ret = __real_pvPortRealloc(pv, size, file, line);
  if (uart_initialized) {
    flush_log_items();
    sj_wdt_feed();
    echo_log_realloc(size, pv, ret, cs_heap_shim);
  } else {
    /* reallocs don't happen before uart is initialized */
    abort();
  }
  cs_heap_shim = 0;
  return ret;
}

void *__wrap_pvPortMalloc(size_t xWantedSize, const char *file, int line) {
  void *ret = __real_pvPortMalloc(xWantedSize, file, line);
  if (uart_initialized) {
    flush_log_items();
    sj_wdt_feed();
    echo_log_malloc(xWantedSize, ret, cs_heap_shim);
  } else {
    add_log_item(ITEM_TYPE_MALLOC, ret, xWantedSize, cs_heap_shim);
  }

  cs_heap_shim = 0;
  return ret;
}

void *__wrap_pvPortZalloc(size_t xWantedSize, const char *file, int line) {
  void *ret = __real_pvPortZalloc(xWantedSize, file, line);
  if (uart_initialized) {
    flush_log_items();
    sj_wdt_feed();
    echo_log_zalloc(xWantedSize, ret, cs_heap_shim);
  } else {
    add_log_item(ITEM_TYPE_ZALLOC, ret, xWantedSize, cs_heap_shim);
  }
  cs_heap_shim = 0;
  return ret;
}

void *__wrap_pvPortCalloc(size_t num, size_t xWantedSize, const char *file,
                          int line) {
  void *ret = __real_pvPortCalloc(num, xWantedSize, file, line);

  if (uart_initialized) {
    flush_log_items();
    sj_wdt_feed();
    echo_log_calloc(xWantedSize, ret, cs_heap_shim);
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

#endif /* ESP_ENABLE_HEAP_TRACE */
