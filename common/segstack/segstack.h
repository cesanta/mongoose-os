/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Segmented stack: a stack implementation which stores cells in separately
 * allocated segments.
 *
 * The implementation allows random cell access, although it's optimized for
 * the top of stack (TOS) to be accessed: accessing TOS is O(1), whereas
 * accessing 0th cell is O(n).
 *
 * Cell and segment sizes are arbitrary and are configurable in runtime, thus
 * segstack can be used in various parts of the system with different
 * configuration. There are no compile-time options. Segstack makes no
 * assumption about the cell type or anything: it operates with plain pointers
 * to cell data.
 */

#ifndef CS_COMMON_SEGSTACK_SEGSTACK_H_
#define CS_COMMON_SEGSTACK_SEGSTACK_H_

#include <stdint.h>

/* SegStack options */
struct segstack_opt {
  /* Size of a single cell, in bytes. */
  uint8_t cell_size;
  /* Size of a segment of cells, in cells. */
  uint8_t seg_size;
  /*
   * How many segments to "stash": when a number of free segments becomes
   * larger, the extra one gets freed. For no stash, set to 0, although that
   * may result in frequent allocations and freeings.
   */
  uint8_t stash_segs;
};

struct segstack {
  /* Options given to segstack_init() */
  struct segstack_opt opt;

  /* Current number of stashed segments (can't be larger than opt.stash_segs) */
  uint8_t stashed_segs;

  /* Number of cells used. Initially, it's 0. */
  int size;

  /* Pointer to the top-of-stack segment */
  struct segstack_seg *tos_seg;

  /*
   * Pointer to the last segment. Needed to implement stashing.
   */
  struct segstack_seg *last_seg;
};

/*
 * Segment of cells. Actual size depends on options.
 */
struct segstack_seg {
  struct segstack_seg *prev;
  uint8_t data[];
};

/*
 * Initializes segstack `ss` with the given options `opt`.
 */
void segstack_init(struct segstack *ss, const struct segstack_opt *opt);

/*
 * Frees all the segments allocated by the stack; the structure `ss` itself
 * is zeroed out and not freed.
 */
void segstack_free(struct segstack *ss);

/*
 * Returns current stack size.
 */
int segstack_size(struct segstack *ss);

/*
 * Sets new stack size. NOTE that size can only be made smaller; an attempt
 * to make it larger will result in a crash.
 */
void segstack_set_size(struct segstack *ss, int size);

/*
 * Returns a pointer to the TOS cell data. If the stack size is 0, returns
 * NULL. Equivalent of `segstack_at(ss, -1)`.
 *
 * NOTE that calling `segstack_pop()` might invalidate the pointer returned
 * by this function.
 */
void *segstack_tos(struct segstack *ss);

/*
 * Returns a pointer to the data of the cell by the given index. If index is
 * out of bounds, returns NULL.
 *
 * Negative index is interpreted as size + idx; thus, TOS can be accessed with
 * the index -1.
 *
 * NOTE that calling `segstack_pop()` might invalidate the pointer returned
 * by this function.
 */
void *segstack_at(struct segstack *ss, int idx);

/*
 * Pushes a new cell on the stack.
 */
void segstack_push(struct segstack *ss, const void *cellp);

/*
 * Pops a cell from the stack. If `cellp` is not NULL, writes the data of the
 * popped cell there.
 *
 * This function can't just return a pointer (as `segstack_tos()` does),
 * because in case of zero `stash_segs`, this pointer will already be invalid.
 *
 * NOTE that calling this function also invalidates pointers previously
 * returned by `segstack_tos()` and `segstack_at()`.
 */
void segstack_pop(struct segstack *ss, void *cellp);

#endif /* CS_COMMON_SEGSTACK_SEGSTACK_H_ */
