/*
 * This source provides the externally available functions for the libdash
 * library, it will provide access to the internal functions as necessary.
 */

#include "shparse/include/parser.h"

#if HAVE_VISIBLE_ATTRIBUTE
#define VISFUNC __attribute__((visibility("default")))
#else
#define VISFUNC
#endif

VISFUNC bool parse_new(struct parse_context **ctx)
{
  if (!ctx) return false;
  return ctx_init(ctx);
}

VISFUNC void parse_free(struct parse_context **ctx)
{
  ctx_fini(ctx);
}

VISFUNC union parse_node *parse_next_command(struct parse_context *ctx)
{
  return ctx_next_command(ctx);
}

VISFUNC const char *parse_internal_errstr(struct parse_context *ctx)
{
  switch (ctx->int_error) {
  case IE_NONE:
    return NULL;

  case IE_NOSOURCE:
    return "No source available";

  case IE_NOUNGET:
    return "No unget function provided";

  case IE_NOGETCHR:
    return "No get function provided";

  default:
    return "Unknown internal error";
  }
}

VISFUNC bool parse_push_string(struct parse_context *ctx, const char *str)
{
  if (!ctx || !str) return false;
  if (!push_source(ctx, SRC_STRING, str)) return false;
  return true;
}

VISFUNC bool parse_push_file(struct parse_context *ctx, const char *fname)
{
  if (!ctx || !fname) return false;
  if (!push_source(ctx, SRC_FILE, fname)) return false;
  return true;
}

VISFUNC bool parse_node_iseof(union parse_node *node)
{
  return node == eof_node();
}
