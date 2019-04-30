/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#pragma once

#include "common/mbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_UPD_SRC_END_OF_DATA -13

struct mgos_ota_src_ctx; /* Defined by data source implementation. */

struct mgos_ota_src_if {
  /*
   * Read should return up to len bytes of data or 0 if no data is available
   * but may become available.
   * Return MGOS_UPD_SRC_END_OF_DATA if there will be no more data.
   * Other negative values are treated as errors and abort the update.
   */
  int (*read)(struct mgos_ota_src_ctx *sctx, void *buf, int len);
  /*
   * Close will be invoked at the end of update with the final result.
   * This may happen earlier than all the data is read if update is aborted.
   * It must take care of releasing resoures and closing the source.
   */
  void (*close)(struct mgos_ota_src_ctx *sctx, int upd_result,
                const char *status_msg);
  /* Should return size of the update package, 0 if not known. */
  int (*size)(struct mgos_ota_src_ctx *sctx);
};

/* Helper to read as much as possible from the mbuf. */
int mgos_ota_src_read_mbuf(struct mbuf *mb, void *buf, int len);

#ifdef __cplusplus
}
#endif
