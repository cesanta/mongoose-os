/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "segstack.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

/*
 * Returns an index of the segment for the given cell index. If negative
 * cellidx is given, -1 is retured.
 */
#define SS_SEG_IDX(ss, cellidx) \
  ((cellidx) >= 0 ? ((cellidx) / (ss)->opt.seg_size) : -1)

/*
 * Returns an index of the cell inside of the segment.
 */
#define SS_SEG_CELL_IDX(ss, cellidx) ((cellidx) % (ss)->opt.seg_size)

/*
 * Returns the size of a segment in bytes.
 */
#define SS_SEG_SIZE_BYTES(ss) \
  (sizeof(struct segstack_seg) + (ss)->opt.cell_size * (ss)->opt.seg_size)

static void *s_segstack_get_cellp(struct segstack *ss, int cellidx,
                                  struct segstack_seg **pseg);
static void s_segstack_free_last_seg(struct segstack *ss);

void segstack_init(struct segstack *ss, const struct segstack_opt *opt) {
  memset(ss, 0, sizeof(*ss));
  memcpy(&ss->opt, opt, sizeof(ss->opt));
  /* TODO(dfrank): check the validity of the options */
}

void segstack_free(struct segstack *ss) {
  while (ss->last_seg != NULL) {
    s_segstack_free_last_seg(ss);
  }
  memset(ss, 0, sizeof(*ss));
}

int segstack_size(struct segstack *ss) {
  return ss->size;
}

void segstack_set_size(struct segstack *ss, int size) {
  assert(segstack_size(ss) >= size);
  while (segstack_size(ss) > size) {
    segstack_pop(ss, NULL);
  }
}

void *segstack_tos(struct segstack *ss) {
  return segstack_at(ss, -1);
}

void *segstack_at(struct segstack *ss, int idx) {
  if (idx < 0) {
    idx = ss->size + idx;
  }

  if (idx < 0 || idx >= ss->size) {
    return NULL;
  }

  return s_segstack_get_cellp(ss, idx, NULL);
}

void segstack_push(struct segstack *ss, const void *cellp) {
  void *tgt_cellp = s_segstack_get_cellp(ss, ss->size, &ss->tos_seg);
  ss->size++;
  memcpy((char *) tgt_cellp, (char *) cellp, ss->opt.cell_size);
}

void segstack_pop(struct segstack *ss, void *cellp) {
  assert(ss->size > 0);
  struct segstack_seg *old_tos_seg = ss->tos_seg;

  /* If the caller provided a pointer to write the popped value to, do it */
  if (cellp != NULL) {
    memcpy(cellp, segstack_tos(ss), ss->opt.cell_size);
  }

  /* Pop the top element from the stack */
  s_segstack_get_cellp(ss, ss->size - 2, &ss->tos_seg);
  ss->size--;

  if (old_tos_seg != ss->tos_seg) {
    /* A new segment just became unused */
    assert(old_tos_seg->prev == ss->tos_seg);

    if (ss->stashed_segs < ss->opt.stash_segs) {
      /* We should stash this unused segment */
      ss->stashed_segs++;
    } else {
      /* No more segments should be stashed, so, free the last one */
      s_segstack_free_last_seg(ss);
    }
  }
}

/*
 * Returns segment by the TOS delta. If `tos_delta` is 0, returns the TOS
 * segment.  If `tos_delta > 0`, returns one of the previous segments.  If
 * `tos_delta` is equal to -1, returns the next segment: eigher previously
 * stashed, or newly allocated.
 *
 * Other values of `tos_delta` are illegal.
 */
static struct segstack_seg *s_segstack_get_seg(struct segstack *ss,
                                               int tos_delta) {
  struct segstack_seg *cur_seg = NULL;
  if (tos_delta >= 0) {
    /* We need to get TOS segment or below */
    cur_seg = ss->tos_seg;
    while (tos_delta-- > 0) {
      cur_seg = cur_seg->prev;
    }
  } else if (tos_delta == -1) {
    /* We need to get the "next" segment after TOS */
    if (ss->stashed_segs > 0) {
      /* The needed segment is already allocated (stashed), we need to find it
       */
      assert(ss->last_seg != NULL);
      assert(ss->last_seg != ss->tos_seg);

      cur_seg = ss->last_seg;
      while (cur_seg->prev != ss->tos_seg) {
        cur_seg = cur_seg->prev;
        assert(cur_seg != NULL);
      }

      ss->stashed_segs--;
    } else {
      /* We need to allocate a new segment */
      ss->last_seg = (struct segstack_seg *) calloc(1, SS_SEG_SIZE_BYTES(ss));
      ss->last_seg->prev = ss->tos_seg;
      cur_seg = ss->last_seg;
    }
  } else {
    abort();
  }
  return cur_seg;
}

/*
 * Returns a pointer to the cell by the given cell index. If `pseg` is not NULL,
 * a pointer to a segment which contains the cell will be written there.
 *
 * Cell index might be in range [-1, ss->size].
 *
 * If cellidx is -1, NULL is returned.
 *
 * If cellidx is equal to `ss->size`, a pointer to the next cell is returned
 * (probably from stashed or newly allocated segment).
 */
static void *s_segstack_get_cellp(struct segstack *ss, int cellidx,
                                  struct segstack_seg **pseg) {
  int tos_seg_idx = SS_SEG_IDX(ss, ss->size - 1);
  int need_seg_idx = SS_SEG_IDX(ss, cellidx);
  int need_seg_cell_idx = SS_SEG_CELL_IDX(ss, cellidx);

  struct segstack_seg *seg = NULL;

  assert(cellidx >= -1 && cellidx <= ss->size);

  seg = s_segstack_get_seg(ss, tos_seg_idx - need_seg_idx);

  if (pseg != NULL) {
    *pseg = seg;
  }

  if (seg != NULL) {
    return seg->data + ss->opt.cell_size * need_seg_cell_idx;
  } else {
    return NULL;
  }
}

/*
 * Frees last segment.
 */
static void s_segstack_free_last_seg(struct segstack *ss) {
  assert(ss->last_seg != NULL);
  struct segstack_seg *t = ss->last_seg;
  ss->last_seg = ss->last_seg->prev;
  free(t);
}
