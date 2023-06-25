/*
 * This source contains the STAILQ equivalent as functions.
 */

#include <stdlib.h>

#if INLINE_QUEUE_FUNCS
#define STAILQ_INLINE static inline
#else
#define STAILQ_INLINE
#endif

struct parse_context;

/* Define a internal header structure */
struct _stailq_elm {
  struct _stailq_elm *next;
};

struct _stailq {
  struct obstack       memstack;
  struct _stailq_elm  *first;
  struct _stailq_elm **last;
  size_t               sizelm;                   /* Size of each element */
};

/* Allocate a new queue and associate it with a new obstack */
STAILQ_INLINE void *stailq_init(struct parse_context *ctx,
                                void                 *que,
                                size_t                sizelm)
{
  struct _stailq *hdr;
  if (sizelm < 1)
    return NULL;
  if (que) {
    hdr = (struct _stailq *)que;
  } else {
    hdr = obstack_alloc(&ctx->memstack, sizeof(struct _stailq));
  }
  obstack_init(&hdr->memstack);
  hdr->first = NULL;
  hdr->last = &hdr->first;
  hdr->sizelm = sizelm;
  return hdr;
}

/* Free the entire queue */
STAILQ_INLINE void stailq_fini(struct parse_context *ctx, void **que)
{
  if (que && *que) {
    struct _stailq *hdr = *(struct _stailq **)que;
    *que = NULL;
    obstack_free(&hdr->memstack, NULL);
    obstack_free(&ctx->memstack, hdr);
  }
}

/* Function to clear a queue */
STAILQ_INLINE void stailq_clear(void *que)
{
  if (que) {
    struct _stailq *hdr = (struct _stailq *)que;
    if (hdr->first) {
      obstack_free(&hdr->memstack, NULL);
      hdr->first = NULL;
      hdr->last = &hdr->first;
    }
  }
}

/* Function to insert an element at head of queue */
STAILQ_INLINE void *stailq_insert_head(void *que, void *elm)
{
  if (que && elm) {
    struct _stailq *hdr = (struct _stailq *)que;
    struct _stailq_elm *ptr = obstack_copy(&hdr->memstack, elm, hdr->sizelm);
    if (!(ptr->next = hdr->first))
      hdr->last = &ptr->next;
    hdr->first = ptr;
    return ptr;
  }
  return NULL;
}

/* Insert at end of queue */
STAILQ_INLINE void *stailq_insert_tail(void *que, void *elm)
{
  if (que && elm) {
    struct _stailq *hdr = (struct _stailq *)que;
    struct _stailq_elm *ptr = obstack_copy(&hdr->memstack, elm, hdr->sizelm);
    ptr->next = NULL;
    *hdr->last = ptr;
    hdr->last = &ptr->next;
    return ptr;
  }
  return NULL;
}

/* Insert after an element */
STAILQ_INLINE void *stailq_insert_after(void *que, void *lst, void *elm)
{
  if (que && lst && elm) {
    struct _stailq *hdr = (struct _stailq *)que;
    struct _stailq_elm *ptr = obstack_copy(&hdr->memstack, elm, hdr->sizelm);
    struct _stailq_elm *lptr = (struct _stailq_elm *)lst;
    if (!(ptr->next = lptr->next))
      hdr->last = &ptr->next;
    lptr->next = ptr;
    return ptr;
  }
  return NULL;
}

/* Remove element from the head of list */
STAILQ_INLINE void *stailq_remove_head(void *que)
{
  if (que) {
    struct _stailq *hdr = (struct _stailq *)que;
    struct _stailq_elm *ptr = (struct _stailq_elm *)hdr->first;

    if (!(hdr->first = hdr->first->next))
      hdr->last = &hdr->first;
    return ptr;
  }
  return NULL;
}

/* Remove a specified element from within list */
STAILQ_INLINE void stailq_remove(void *que, void *elm)
{
  if (que && elm) {
    struct _stailq *hdr = (struct _stailq *)que;
    struct _stailq_elm *ptr = (struct _stailq_elm *)elm;
    if (hdr->first == elm) {
      if (!(hdr->first = hdr->first->next))
        hdr->last = &hdr->first;
    } else {
      struct _stailq_elm *cptr;
      for (cptr = hdr->first; cptr != ptr; cptr = cptr->next);
      if (cptr && !(cptr->next = cptr->next->next))
        hdr->last = &cptr->next;
    }
  }
}

/* Concat one list to the tail of another */
STAILQ_INLINE void stailq_concat(void *que1, void *que2)
{
  if (que1 && que2) {
    struct _stailq *hdr1 = (struct _stailq *)que1;
    struct _stailq *hdr2 = (struct _stailq *)que2;
    if (hdr1->sizelm == hdr2->sizelm) {
      struct _stailq_elm *ptr;
      for (ptr = hdr2->first; ptr; ptr = ptr->next) {
        stailq_insert_tail(que1, ptr);
      }
    }
  }
}

/* Copy one list over the top of another */
STAILQ_INLINE void stailq_copy(void *que1, void *que2)
{
  if (que1 && que2) {
    struct _stailq *hdr1 = (struct _stailq *)que1;
    struct _stailq *hdr2 = (struct _stailq *)que2;
    if (hdr1->sizelm == hdr2->sizelm) {
      struct _stailq_elm *ptr;
      obstack_free(&hdr1->memstack, NULL);
      for (ptr = hdr2->first; ptr; ptr = ptr->next) {
        stailq_insert_tail(que1, ptr);
      }
    }
  }
}

STAILQ_INLINE bool stailq_empty(void *que)
{
  if (que) {
    struct _stailq *hdr = (struct _stailq *)que;
    return (hdr->last == &hdr->first);
  } else {
    return true;
  }
}

STAILQ_INLINE void *stailq_head(void *que)
{
  if (que) {
    struct _stailq *hdr = (struct _stailq *)que;
    return hdr->first;
  } else {
    return NULL;
  }
}

STAILQ_INLINE void *stailq_tail(void *que)
{
  if (que) {
    struct _stailq *hdr = (struct _stailq *)que;
    return *hdr->last;
  } else {
    return NULL;
  }
}
#undef STAILQ_INLINE
