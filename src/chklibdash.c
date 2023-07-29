/*
 * Test program for the libdash library.
 */

#include "libdash.h"
#include <stdio.h>

/* Configure the options being used by program */
#define USE_FULL_TEST     0         /* Use individual tests */
#if USE_FULL_TEST
/*CHKLIBDASH_DUMP*/
/*CHKLIBDASH_STRING*/
/*USE_WALKINFO*/
#endif

#if USE_FULL_TEST
# ifdef CHKLIBDASH_STRING
  char *cmd = ";";                                 /* T000 */ 
/*char *cmd = "echo";                               * T001 */
/*char *cmd = "for a in 1 3 5; do func $a; done";   * T002 */
# else
char *fname = "excmd";
# endif

int main(void) {
# ifdef USE_WALKINFO
  struct parse_walkinfo *wi;
# else
  union parse_node *node;
# endif
  struct parse_context *ctx = NULL;
  const char *interr;

  if (!parse_new(&ctx)) {
    puts("FAILED CONTEXT");
    return 1;
  }
# ifdef CHKLIBDASH_STRING
  if (!parse_push_string(ctx, cmd)) {
    puts("FAILED PUSHSTR");
    parse_free(&ctx);
    return 2;
  }
# else
  if (!parse_push_file(ctx, fname)) {
    puts("FAILED PUSHFILE");
    parse_free(&ctx);
    return 2;
  }
# endif
# ifdef USE_WALKINFO
  if (!(wi = parse_next_command(ctx))) {
    puts("FAILED PARSER");
    parse_free(&ctx);
    return 3;
  }
  if (parse_iseof(wi)) {
    puts("NODE: EOF");
#  ifdef CHKLIBDASH_DUMP /* ifndef NDEBUG */
  } else {
    parse_node_dump("NODE:", wi);
#  else
  } else {
    printf("NODE: %p\n", wi);
#  endif
  }
# else /*!USE_WALKINFO*/
  if (!(node = parse_next_command(ctx))) {
    puts("FAILED PARSER");
    parse_free(&ctx);
    return 3;
  }
  if (parse_iseof(node)) {
    puts("NODE: EOF");
  } else {
    printf("NODE: %p\n", node);
  }
# endif
  if ((interr = parse_internal_errstr(ctx)))
    printf("INTERNAL: %s\n", interr);
  parse_free(&ctx);
  puts("SUCCESS");
  return 0;
}
#else /*!USE_FULL_TEST*/
static const char *tokid_str(enum parse_tokid tokid)
{
  switch (tokid) {
    case TEOF:       return "TEOF";
    case TNL:        return "TNL";
    case TSEMI:      return "TSEMI";
    case TBACKGND:   return "TBACKGND";
    case TAND:       return "TAND";
    case TOR:        return "TOR";
    case TPIPE:      return "TPIPE";
    case TLP:        return "TLP";
    case TRP:        return "TRP";
    case TENDCASE:   return "TENDCASE";
    case TENDBQUOTE: return "TENDBQUOTE";
    case TREDIR:     return "TREDIR";
    case TWORD:      return "TWORD";
    case TNOT:       return "TNOT";
    case TCASE:      return "TCASE";
    case TDO:        return "TDO";
    case TDONE:      return "TDONE";
    case TELIF:      return "TELIF";
    case TELSE:      return "TELSE";
    case TESAC:      return "TESAC";
    case TFI:        return "TFI";
    case TFOR:       return "TFOR";
    case TIF:        return "TIF";
    case TIN:        return "TIN";
    case TTHEN:      return "TTHEN";
    case TUNTIL:     return "TUNTIL";
    case TWHILE:     return "TWHILE";
    case TBEGIN:     return "TBEGIN";
    case TEND:       return "TEND";
    default:         break;
  }
  return "INVALID";
};

static size_t numtest = 1;
static bool push_and_tokenise(struct parse_context *ctx, const char *cmd)
{
  enum parse_tokid tokid;

  if (cmd && !parse_push_string(ctx, cmd)) {
    printf("%zd: FAILED PUSHSTR: %s\n", numtest, cmd);
    return false;
  }
  if ((tokid = parse_next_token(ctx)) == INV_PARSER_TOKEN) {
    printf("%zd: INVALID TOKEN\n", numtest);
    return false;
  }
  printf("%zd: TOKEN: %d (%s)\n", numtest, tokid, tokid_str(tokid));
  numtest++;
  return true;
}

/* Perform the tokeniser tests */
# define PUSH_TOKEN(cmd) \
  if (!push_and_tokenise(ctx, cmd)) { ret=numtest; goto finish; }

int main(void) {
  struct parse_context *ctx = NULL;
  int ret = 0;

  if (!parse_new(&ctx)) {
    puts("FAILED CONTEXT");
    return 1;
  }
  PUSH_TOKEN(";");
  PUSH_TOKEN(";;");
  PUSH_TOKEN(";\n;");
  PUSH_TOKEN(NULL);
  PUSH_TOKEN(NULL);
  PUSH_TOKEN(NULL);
  PUSH_TOKEN(NULL);
finish:
  parse_free(&ctx);
  return ret;
}
# undef PUSH_TOKEN
#endif /*!USE_FULL_TEST*/
