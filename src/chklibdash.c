/*
 * Test program for the libdash library.
 */

#include "libdash.h"
#include <stdio.h>

#ifdef CHKLIBDASH_STRING
char *cmd = "echo";
#else
char *fname = "excmd";
#endif

int main(void) {
  union parse_node *node;
  struct parse_context *ctx = NULL;
  const char *interr;

  if (!parse_new(&ctx)) {
    puts("FAILED CONTEXT");
    return 1;
  }
#ifdef CHKLIBDASH_STRING
  if (!parse_push_string(ctx, cmd)) {
    puts("FAILED PUSHSTR");
    parse_free(&ctx);
    return 2;
  }
#else
  if (!parse_push_file(ctx, fname)) {
    puts("FAILED PUSHFILE");
    parse_free(&ctx);
    return 2;
  }
#endif
  if (!(node = parse_next_command(ctx))) {
    puts("FAILED PARSER");
    parse_free(&ctx);
    return 3;
  }
  if (!parse_node_iseof(node)) {
    printf("NODE: %p\n", node);
  } else {
    puts("NODE: EOF");
  }
  if ((interr = parse_internal_errstr(ctx)))
    printf("INTERNAL: %s\n", interr);
  parse_free(&ctx);
  puts("SUCCESS");
  return 0;
}
