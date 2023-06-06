/*
 * This file provides the functions and other structures that provide access
 * to the current source file being parsed.
 */

#include <obstack.h>
#include <stdlib.h>
#include "parser.h"
#include "queue.h"

/* Provide the possible flag values used internally */
enum srcflag {
  SF_FALSE = 0,                /* False value for bool functions */
  SF_TRUE,                     /* True value for bool functions */
  SF_NODATA,                   /* No further data available */
  SF_ERROR,                    /* Error occured within functions */
};

struct parse_source_ops {
  enum srcflag (*init)(struct parse_source_hdr *, struct parse_source *);
  enum srcflag (*fini)(struct parse_source_hdr *, struct parse_source *);
  enum srcflag (*read_char)(struct parse_source *, char *);
  enum srcflag (*unget_char)(struct parse_source *, char);
  enum srcflag (*tell)(struct parse_source *, off_t *);
  enum srcflag (*seek)(struct parse_source *, off_t);
  enum srcflag (*open)(struct parse_source *, void *);
  enum srcflag (*close)(struct parse_source *);
  /* ... */
};

#define MAX_UNGOT 4
struct parse_source {
  struct parse_source     *next;             /* Previous source in list */
  struct parse_source_ops *ops;              /* Related operations */
  struct _source_ungot {
    char                   data[MAX_UNGOT];  /* Ungot data */
    size_t                 curpos;           /* Length of ungot data */
  } ungot;
  struct _source_data {
    void                  *data;             /* Underlying data */
    off_t                  baseoff;          /* Base offset for position */
    size_t                 remain;           /* Amount of data remaining */
    size_t                 curpos;           /* Current position within data */
  } data;
  unsigned int             lineno;           /* Current line within data */
  bool                     isclosed;         /* Set if source has been closed */
  /* ... */
};

#if defined(NEED_DUMMY_OPS) && NEED_DUMMY_OPS
/* Provide some dummy functions that can be used in place of open/close */
static enum srcflag dum_init(struct parse_source_hdr *hdr,
                             struct parse_source *src)
{
  return SF_TRUE;
}
static enum srcflag dum_fini(struct parse_source_hdr *hdr,
                             struct parse_source *src)
{
  return SF_TRUE;
}
static enum srcflag dum_read_char(struct parse_source *src, char *chr)
{
  return SF_TRUE;
}
static enum srcflag dum_unget_char(struct parse_source *src, char)
{
  return SF_TRUE;
}
static enum srcflag dum_tell(struct parse_source *src, off_t *pos)
{ 
  return SF_TRUE;
}
static enum srcflag dum_seek(struct parse_source *src, off_t pos)
{ 
  return SF_TRUE;
}
static enum srcflag dum_open(struct parse_source *src, void *data)
{ 
  return SF_TRUE; 
}
static enum srcflag dum_close(struct parse_source *src)
{
  return SF_TRUE;
}
#endif

/* Define common operation functions */
static enum srcflag cmn_init(struct parse_source_hdr *hdr,
                             struct parse_source *src)
{
  src->ungot.curpos = 0;
  src->data.baseoff = 0;
  src->data.curpos = 0;
  return SF_TRUE;
}

/* Define the operations that can be performed on a string source */
static enum srcflag str_read_char(struct parse_source *src, char *chr)
{
  if (!src || !chr ) return SF_FALSE;
  if (src->ungot.curpos > 0) {
    /* Attempt to use ungot data first */
    *chr = src->ungot.data[--src->ungot.curpos];
  } else if (src->isclosed) {
    /* Unable to fetch more data if it has been closed */
    return SF_FALSE;
  } else if (!src->data.remain) {
    return SF_NODATA;
  } else {
    /* Use the next character from the data object */
    *chr = *((char *)src->data.data + src->data.baseoff + src->data.curpos++);
    src->data.remain--;
  }
  return SF_TRUE;
}
static enum srcflag str_unget_char(struct parse_source *src, char chr)
{
  if (!src) {
    return SF_FALSE;
  } else if (!src->isclosed && src->data.curpos) {
    /* Move back in the actual data */
    src->data.curpos--;
    src->data.remain++;
  } else if (src->ungot.curpos < MAX_UNGOT) {
    /* Add to the ungot buffer */
    src->ungot.data[src->ungot.curpos++] = chr;
  } else {
    /* Ungot buffer is full */
    return SF_FALSE;
  }
  return SF_TRUE;
}
static enum srcflag str_tell(struct parse_source *src, off_t *off)
{
  if (!src || !off) return SF_FALSE;
  if (src->isclosed) {
    if (src->ungot.curpos) {
      *off = src->ungot.curpos;
    } else {
      return SF_FALSE;
    }
  } else if (src->data.remain) {
    *off = src->data.baseoff + src->data.curpos;
  } else {
    return SF_FALSE;
  }
  return SF_TRUE;
}
static enum srcflag str_seek(struct parse_source *src, off_t off)
{
  if (!src || src->isclosed || off > src->data.remain)
    return SF_FALSE;
  src->data.curpos = off - src->data.baseoff;
  return SF_TRUE;
}
static enum srcflag str_open(struct parse_source *src, void *data)
{
  if (!src || !data) return SF_FALSE;
  src->data.data = data;
  src->data.remain = strlen((char *)data);
  src->data.baseoff = 0;
  src->data.curpos = 0;
  src->isclosed = false;
  return SF_TRUE;
}
static enum srcflag str_close(struct parse_source *src)
{
  if (!src || !src->data.data || src->isclosed) return SF_FALSE;
  src->isclosed = true;
  return SF_TRUE;
}

/* Define the operations that can be performed on a file source */
static enum srcflag fyl_read_char(struct parse_source *src, char *chr)
{
  if (!src || !chr) return SF_FALSE;
  if (src->ungot.data && src->ungot.curpos > 0) {
    /* Attempt to use ungot data first */
    *chr = *((char *)src->ungot.data + src->ungot.curpos);
    src->ungot.curpos--;
  } else if (src->isclosed) {
    /* Unable to fetch more data if it has been closed */
    return SF_FALSE;
  } else if (!src->data.remain) {
    return SF_NODATA;
  } else {
    /* Use the next character from the data object */
    if (fread(chr, 1, 1, src->data.data) != 1) return SF_NODATA;
    src->data.remain--;
    src->data.curpos++;
    /* ... Implement ... */
  }
  return SF_TRUE;
}
static enum srcflag fyl_unget_char(struct parse_source *src, char chr)
{
  if (!src) {
    return SF_FALSE;
  } else if (src->data.remain) {
    src->data.remain++;
    src->data.curpos--;
  } else if (src->ungot.curpos < MAX_UNGOT) {
    src->ungot.data[src->ungot.curpos++] = chr;
  } else {
    return SF_FALSE;
  }
  return SF_TRUE;
}
static enum srcflag fyl_tell(struct parse_source *src, off_t *off)
{
  if (!src || !off || !src->data.data || src->isclosed) return SF_FALSE;
  /* ... */
  return SF_TRUE;
}
static enum srcflag fyl_seek(struct parse_source *src, off_t off)
{
  if (!src || src->isclosed || off > src->data.remain)
    return SF_FALSE;
  /* ... */
  return SF_TRUE;
}
static enum srcflag fyl_open(struct parse_source *src, void *data)
{
  const char *fname;
  FILE *fd;

  if (!src || !data) return SF_FALSE;
  fname = (const char *)data;
  if (!(fd = fopen(fname, "rt"))) return SF_FALSE;;
  src->data.data = fd;
  src->data.baseoff = 0;
  src->data.curpos = 0;
  fseek(fd, 0, SEEK_END);
  src->data.remain = ftell(fd);
  fseek(fd, 0, SEEK_SET);
  src->ungot.curpos = 0;
  src->isclosed = false;
  return SF_TRUE;
}
static enum srcflag fyl_close(struct parse_source *src)
{
  FILE *fd;

  if (!src || !src->data.data || src->isclosed) return SF_FALSE;
  src->isclosed = true;
  fd = (FILE *)src->data.data;
  src->data.data = NULL;
  src->data.remain = 0;
  fclose(fd);
  return SF_TRUE;
}

int init_source(struct parse_context *ctx)
{
  if (!ctx->lst_source) {
    ctx->lst_source = obstack_alloc(&ctx->memstack,
        sizeof(struct parse_source_hdr));
  }
  stailq_init(ctx, ctx->lst_source, sizeof(struct parse_source));
  return 0;
}

int fini_source(struct parse_context *ctx)
{
  stailq_clear(ctx->lst_source);
  return 0;
}

/* Define the known operations provided */
static struct parse_source_ops k_ops[SRC_NUM] = {
  [SRC_STRING] = {       /* String based operations */
    .init = cmn_init,
    .read_char = str_read_char,
    .unget_char = str_unget_char,
    .tell = str_tell,
    .seek = str_seek,
    .open = str_open,
    .close = str_close
  },
  [SRC_FILE] = {         /* File based operations */
    .init = cmn_init,
    .read_char = fyl_read_char,
    .unget_char = fyl_unget_char,
    .tell = fyl_tell,
    .seek = fyl_seek,
    .open = fyl_open,
    .close = fyl_close
  }
#if defined(NEED_DUMMY_OPS) && NEED_DUMMY_OPS
  [SRC_DUMMY] = {                /* Dummy operations */
    .init = dum_init,
    .fini = dum_fini,
    .read_char = dum_read_char,
    .unget_char = dum_unget_char,
    .tell = dum_tell,
    .seek = dum_seek,
    .open = dum_open,
    .close = dum_close
  }
#endif
};

/* Allocate a new source and add it to the stack */
struct parse_source *push_source(struct parse_context *ctx,
                                 enum parse_srctype    type,
                                 void const           *data)
{
  struct parse_source_hdr *hdr = ctx->lst_source;

  if (type < SRC_NUM) {
    struct parse_source *node = obstack_alloc(&hdr->memstack,
        sizeof(struct parse_source));
    node->ops = &k_ops[type];
    if (node->ops->init)
      node->ops->init(hdr, node);
    if (node->ops->open(node, data)) {
      stailq_insert_head(hdr, node);
      return node;
    }
    obstack_free(&hdr->memstack, node);
  }
  return NULL;
}

/* Remove the last source from the stack of sources */
struct parse_source *pop_source(struct parse_context *ctx)
{
  struct parse_source_hdr *hdr = ctx->lst_source;
  if (hdr) {
    struct parse_source *fre = hdr->first;
    stailq_remove_head(hdr);
    fre->ops->close(fre);
    obstack_free(&hdr->memstack, fre);
    return hdr->first;
  } else {
    return NULL;
  }
}

char source_next_char(struct parse_context *ctx)
{
  struct parse_source_hdr *hdr;
  struct parse_source *src;
  char chr = '\0';

  if (!(hdr = ctx->lst_source) || !(src = hdr->first)) {
    ctx->int_error = IE_NOSOURCE;
    return '\0';
  } else {
    enum srcflag sf;
    while ((sf = src->ops->read_char(src, &chr)) == SF_NODATA) {
      if (!(src = pop_source(ctx)))
        return '\0';
    }
    switch (sf) {
    case SF_ERROR:
      ctx->int_error = IE_NOGETCHR;
      return '\0';
    case SF_FALSE:
      return '\0';
    case SF_TRUE:
      if (chr == '\n')
        src->lineno++;
      return chr;
    default:
      break;
    }
  }
  /* .. Implement .. */
  return chr;
}

void source_unget_char(struct parse_context *ctx, char chr)
{
  struct parse_source *src;

  if (!(src = ctx->lst_source->first)) {
    ctx->int_error = IE_NOSOURCE;
  } else if (!src->ops->unget_char(src, chr)) {
    ctx->int_error = IE_NOUNGET;
  }
  /* .. Implement .. */
  return;
}

unsigned int source_currline(struct parse_context *ctx)
{
  struct parse_source *src;

  if (!(src = ctx->lst_source->first)) {
    ctx->int_error = IE_NOSOURCE;
    return 0;
  }
  return src->lineno;
}
