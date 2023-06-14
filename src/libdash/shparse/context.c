/*
 * This source provides context related functions that remove the need to know
 * implementation details by the application program.
 */

#include <obstack.h>
#include <stdlib.h>
#include "parser.h"
#include "queue.h"

/*
 * Initialise a new context including the related obstack and internal lists
 */
bool ctx_init(struct parse_context **ctx)
{
  struct parse_context *new;

  if (!ctx) return false;
  if (*ctx) {
    /* Re-initialise existing context */
    new = *ctx;
    stailq_clear(new->lst_source);
    dtailq_clear(new->lst_syntax);
    stailq_clear(new->lst_heredoc);
    stailq_clear(new->backquote);
    obstack_free(&new->memstack, NULL);
    obstack_free(&new->txtstack, NULL);
  } else {
    /* Initialise a new context */
    if (!(new = malloc(sizeof(struct parse_context)))) return false;
    memset(new, 0, sizeof(struct parse_context));
    obstack_init(&new->memstack);
    obstack_init(&new->txtstack);
    init_source(new);
    /*new->lst_syntax = dtailq_init(new, NULL, sizeof(struct parse_syntax));  -- defer to actual usage */
    new->lst_heredoc = stailq_init(new, NULL, sizeof(struct parse_heredoc));
    new->backquote = stailq_init(new, NULL, sizeof(struct parse_nodelist));
    *ctx = new;
  }
  return true;
}

void ctx_fini(struct parse_context **ctx)
{
  struct parse_context *fre;
  if (!ctx || !*ctx) return;
  fre = *ctx;
  *ctx = NULL;
  fini_source(fre);
  dtailq_clear(fre->lst_syntax);
  stailq_clear(fre->lst_heredoc);
  stailq_clear(fre->backquote);
  obstack_free(&fre->txtstack, NULL);
  obstack_free(&fre->memstack, NULL);
  free(fre);
}

#if NEED_CTX_STRDUP_REPLACE
void ctx_strdup_replace(struct parse_context *ctx, char **dst, char *src)
{
  if (src && dst) {
    size_t srclen = strlen(src);
    if (*dst) {
      size_t dstlen = strlen(*dst);
      if (dstlen != srclen) {
        obstack_free(&ctx->memstack, *dst);
        *dst = obstack_copy0(&ctx->memstack, src, srclen);
      } else {
        strcpy(*dst, src);
      }
    } else {
      *dst = obstack_copy0(&ctx->memstack, src, srclen);
    }
  }
}
#endif

char *ctx_strdup(struct parse_context *ctx, char *src)
{
  char *dst = NULL;
  if (src) {
    size_t srclen = strlen(src);
    dst = obstack_copy0(&ctx->memstack, src, srclen);
  } else {
    dst = obstack_copy(&ctx->memstack, "\0", 1);
  }
  return dst;
}

void set_synerror(struct parse_context  *ctx,
                  enum parse_synerrcode  code,
                  struct parse_token    *token,
                  char                  *errtext)
{
  if (ctx) {
    ctx->synerror.code = code;
    if (code == SE_EXPECTED && token)
      ctx->synerror.token = *token;
    if (errtext)
      ctx->synerror.errtext = ctx_strdup(ctx, errtext);
  }
}

void clr_synerror(struct parse_context *ctx)
{
  if (ctx) {
    ctx->synerror.code = SE_NONE;
    if (ctx->synerror.errtext)
      obstack_free(&ctx->memstack, ctx->synerror.errtext);
  }
}

void ctx_synerror_expect(struct parse_context *ctx,
                         enum parse_tokid      tokid)
{
  if (ctx && tokid != INV_TOKEN) {
    ctx->synerror.code = SE_EXPECTED;
    ctx->synerror.token.id = tokid;
    if (ctx->synerror.errtext) {
      obstack_free(&ctx->memstack, ctx->synerror.errtext);
      ctx->synerror.errtext = NULL;
    }
  }
}

void ctx_synerror(struct parse_context  *ctx,
                  enum parse_synerrcode  errcode,
                  enum parse_tokid       tokid,
                  char                  *errtext)
{
  if (ctx) {
    ctx->synerror.code = errcode;
    ctx->synerror.token.id = tokid;
    if (ctx->synerror.errtext)
      obstack_free(&ctx->memstack, ctx->synerror.errtext);
    if (errtext)
      ctx->synerror.errtext = obstack_copy0(&ctx->memstack, errtext, strlen(errtext));
  }
}
