/*
 * This source contains the TAILQ equivalent as functions.
 */

#include <stdlib.h>

#if INLINE_QUEUE_FUNCS
#define DTAILQ_INLINE static inline
#else
#define DTAILQ_INLINE
#endif

struct parse_context;

/* Define a internal header structure */
struct _dtailq_elm {
  struct _dtailq_elm *next;
  struct _dtailq_elm **prev;
};

struct _dtailq {
  struct obstack       memstack;
  struct _dtailq_elm  *first;
  struct _dtailq_elm **last;
  size_t               sizelm;
};

/* Allocate a new queue and associate it with a new obstack */
DTAILQ_INLINE void *dtailq_init(struct parse_context *ctx, 
                                void                 *que,
                                size_t                sizelm)
{
  struct _dtailq *hdr;
  if (sizelm < 1)
    return NULL;
  if (que) {
    hdr = (struct _dtailq *)que;
  } else {
    hdr = obstack_alloc(&ctx->memstack, sizeof(struct _dtailq));
  }
  obstack_init(&hdr->memstack);
  hdr->first = NULL;
  hdr->last = &hdr->first;
  hdr->sizelm = sizelm;
  return hdr;
}

/* Free the entire queue */
DTAILQ_INLINE void dtailq_fini(struct parse_context *ctx, void **que)
{
  if (ctx && que && *que) {
    struct _dtailq *hdr = *que;
    *que = NULL;
    obstack_free(&hdr->memstack, NULL);
    obstack_free(&ctx->memstack, hdr);
  }
}

/* Function to clear a queue */
DTAILQ_INLINE void dtailq_clear(void *que)
{
  if (que) {
    struct _dtailq *hdr = (struct _dtailq *)que;
    if (hdr->first) {
      obstack_free(&hdr->memstack, NULL);
      hdr->first = NULL;
      hdr->last = &hdr->first;
    }
  }
}

/* Function to insert an element at head of queue */
DTAILQ_INLINE void *dtailq_insert_head(void *que, void *elm)
{
  if (que && elm) {
    struct _dtailq *hdr = (struct _dtailq *)que;
    struct _dtailq_elm *ptr = obstack_copy(&hdr->memstack, elm, hdr->sizelm);
    if (!(ptr->next = hdr->first)) {
      hdr->last = &ptr->next;
    } else {
      hdr->first->prev = &ptr->next;
    }
    hdr->first = ptr;
    ptr->prev = &hdr->first;
    return ptr;
  }
  return NULL;
}

/* Insert at end of queue */
DTAILQ_INLINE void *dtailq_insert_tail(void *que, void *elm)
{
  if (que && elm) {
    struct _dtailq *hdr = (struct _dtailq *)que;
    struct _dtailq_elm *ptr = obstack_copy(&hdr->memstack, elm, hdr->sizelm);
    ptr->next = NULL;
    ptr->prev = hdr->last;
    *hdr->last = ptr;
    hdr->last = &ptr->next;
    return ptr;
  }
  return NULL;
}

/* Insert after an element */
DTAILQ_INLINE void *dtailq_insert_after(void *que, void *lelm, void *elm)
{
  if (que && lelm && elm) {
    struct _dtailq *hdr = (struct _dtailq *)que;
    struct _dtailq_elm *ptr = obstack_copy(&hdr->memstack, elm, hdr->sizelm);
    struct _dtailq_elm *lptr = (struct _dtailq_elm *)lelm;
    if (!(ptr->next = lptr->next)) {
      hdr->last = &ptr->next;
    } else {
      ptr->next->prev = &ptr->next;
    }
    lptr->next = ptr;
    ptr->prev = &lptr->next;
    return ptr;
  }
  return NULL;
}

/* Insert before an element */
DTAILQ_INLINE void *dtailq_insert_before(void *lelm,  void *elm)
{
  if (lelm && elm) {
    struct _dtailq_elm *lptr = (struct _dtailq_elm *)lelm;
    struct _dtailq_elm *ptr = (struct _dtailq_elm *)elm;
    ptr->prev = lptr->prev;
    ptr->next = lptr;
    *lptr->prev = ptr;
    lptr->prev = &ptr->next;
    return ptr;
  }
  return NULL;
}

/* Remove the element from the head of the list */
DTAILQ_INLINE void *dtailq_remove_head(void *que)
{
  if (que) {
    struct _dtailq *hdr = (struct _dtailq *)que;
    if (hdr->first) {
      struct _dtailq_elm *ptr = hdr->first;
      ptr->next->prev = NULL;
      hdr->first = ptr->next;
      return ptr;
    }
  }
  return NULL;
}

/* Remove the element from the tail of the list */
DTAILQ_INLINE void *dtailq_remove_tail(void *que)
{
  if (que) {
    struct _dtailq *hdr = (struct _dtailq *)que;
    if (*hdr->last) {
      struct _dtailq_elm *ptr = *hdr->last;
      struct _dtailq_elm *prv = *ptr->prev;
      prv->next = NULL;
      ptr->prev = NULL;
      hdr->last = &prv->next;
      return ptr;
    }
  }
  return NULL;
}

/* Remove a specified element from within list */
DTAILQ_INLINE void dtailq_remove(void *que, void *elm)
{
  if (que && elm) {
    struct _dtailq *hdr = (struct _dtailq *)que;
    struct _dtailq_elm *ptr = (struct _dtailq_elm *)elm;
    if (!ptr->next) {
      hdr->last = ptr->prev;
    } else {
      ptr->next->prev = ptr->prev;
    }
    *ptr->prev= ptr->next;
  }
}

/* Concat one list to the tail of another */
DTAILQ_INLINE void dtailq_concat(void *que1, void *que2)
{
  if (que1 && que2) {
    struct _dtailq *hdr1 = (struct _dtailq *)que1;
    struct _dtailq *hdr2 = (struct _dtailq *)que2;
    if (hdr1->sizelm == hdr2->sizelm) {
      struct _dtailq_elm *ptr;
      for (ptr = hdr2->first; ptr; ptr = ptr->next) {
        dtailq_insert_tail(que1, ptr);
      }
    }
  }
}

/* Copy one list over the top of another */
DTAILQ_INLINE void dtailq_copy(void *que1, void *que2)
{
  if (que1 && que2) {
    struct _dtailq *hdr1 = (struct _dtailq *)que1;
    struct _dtailq *hdr2 = (struct _dtailq *)que2;
    if (hdr1->sizelm == hdr2->sizelm) {
      struct _dtailq_elm *ptr;
      obstack_free(&hdr1->memstack, NULL);
      for (ptr = hdr2->first; ptr; ptr = ptr->next) {
        dtailq_insert_tail(que1, ptr);
      }
    }
  }
}

DTAILQ_INLINE bool dtailq_empty(void *que)
{
  if (que) {
    struct _dtailq *hdr = (struct _dtailq *)que;
    return (hdr->last == &hdr->first);
  } else {
    return false;
  }
}

DTAILQ_INLINE void *dtailq_head(void *que)
{
  if (que) {
    struct _dtailq *hdr = (struct _dtailq *)que;
    return hdr->first;
  } else {
    return NULL;
  }
}

DTAILQ_INLINE void *dtailq_tail(void *que)
{
  if (que) {
    struct _dtailq *hdr = (struct _dtailq *)que;
    return *hdr->last;
  } else {
    return NULL;
  }
}
#undef DTAILQ_INLINE
