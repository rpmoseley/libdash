/*
 * This header provides the definitions for the queue functions.
 */

#pragma once

#if INLINE_QUEUE_FUNCS
#include "stailq.h"
#include "dtailq.h"
#else
extern void *stailq_init(struct parse_context *ctx, void *que, size_t sizelm);
extern void stailq_fini(struct parse_context *ctx, void **que);
extern void stailq_clear(void *que);
extern void *stailq_insert_head(void *que, void *elm);
extern void *stailq_insert_tail(void *que, void *elm);
extern void *stailq_insert_after(void *que, void *lst, void *elm);
extern void *stailq_remove_head(void *que);
extern void stailq_remove(void *que, void *elm);
extern void stailq_concat(void *que1, void *que2);
extern void stailq_copy(void *que1, void *que2);
extern bool stailq_empty(void *que);

extern void *dtailq_init(struct parse_context *ctx, void *que, size_t sizelm);
extern void dtailq_fini(struct parse_context *ctx, void **que);
extern void dtailq_clear(void *que);
extern void *dtailq_insert_head(void *que, void *elm);
extern void *dtailq_insert_tail(void *que, void *elm);
extern void *dtailq_insert_after(void *que, void *lelm, void *elm);
extern void *dtailq_insert_before(void *lelm,  void *elm);
extern void *dtailq_remove_head(void *que);
extern void *dtailq_remove_tail(void *que);
extern void dtailq_remove(void *que, void *elm);
extern void dtailq_concat(void *que1, void *que2);
extern void dtailq_copy(void *que1, void *que2);
extern bool dtailq_empty(void *que);
#endif

/* Define some shared macros */
#define STAILQ_HEAD(name, type) \
struct parse_##name {           \
  struct obstack memstack;      \
  struct parse_##type *first;   \
  struct parse_##type **last;   \
  size_t sizelm;                \
}

#define STAILQ_FOREACH(var, que) \
  for ((var) = ((que)->first);   \
       (var);                    \
       (var) = ((var)->next))

#define DTAILQ_HEAD(name, type) \
  STAILQ_HEAD(name, type)

#define DTAILQ_FOREACH(var, que) \
  STAILQ_FOREACH(var, que)

#define DTAILQ_FOREACH_REVERSE(var, que) \
  for ((var) = *((que)->last);           \
       (var);                            \
       (var) = *((var)->prev))
