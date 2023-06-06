/*
 * This file provides the tokenizer logic.
 */

#include <stddef.h>
#include <stdlib.h>
#include "parser.h"
#include "queue.h"

typedef struct tokinfo {
	const char       *name;         /* Name of token given on error/warning */
	const char       *kwd;          /* Keyword or NULL if not */
	enum parse_tokid  id;           /* Token ID from token.h */
	bool              endlist;      /* Token marks end of list */
} tokinfo_t;

/* Define the list of possible tokens and keywords that can be produced */
static struct tokinfo tokinfo[] = {
	{ "end of file", NULL,    TEOF,       true },
	{ "newline",     NULL,    TNL,        false },
	{ "\";\"",       NULL,    TSEMI,      false },
	{ "\"&\"",       NULL,    TBACKGND,   false },
	{ "\"&&\"",      NULL,    TAND,       false },
	{ "\"||\"",      NULL,    TOR,        false },
	{ "\"|\"",       NULL,    TPIPE,      false },
	{ "\"(\"",       NULL,    TLP,        false },
	{ "\")\"",       NULL,    TRP,        true  },
	{ "\";;\"",      NULL,    TENDCASE,   true  },
	{ "\"`\"",       NULL,    TENDBQUOTE, true  },
	{ "redirection", NULL,    TREDIR,     false },
	{ "word",        NULL,    TWORD,      false },
	{ "\"!\"",       "!",     TNOT,       false },
	{ "\"case\"",    "case",  TCASE,      false },
	{ "\"do\"",      "do",    TDO,        true  },
	{ "\"done\"",    "done",  TDONE,      true  },
	{ "\"elif\"",    "elif",  TELIF,      true  },
	{ "\"else\"",    "else",  TELSE,      true  },
	{ "\"esac\"",    "esac",  TESAC,      true  },
	{ "\"fi\"",      "fi",    TFI,        true  },
	{ "\"for\"",     "for",   TFOR,       false },
	{ "\"if\"",      "if",    TIF,        false },
	{ "\"in\"",      "in",    TIN,        false },
	{ "\"then\"",    "then",  TTHEN,      true  },
	{ "\"until\"",   "until", TUNTIL,     false },
	{ "\"while\"",   "while", TWHILE,     false },
	{ "\"{\"",       "{",     TBEGIN,     false },
	{ "\"}\"",       "}",     TEND,       true  }
};

/* Provide access to the token list */
bool endtoklist(enum parse_tokid tokid)
{
  return tokinfo[tokid].endlist;
}

/* Check if given token is a keyword */
bool iskeyword(enum parse_tokid tokid)
{
  return tokinfo[tokid].kwd != NULL;
}

/* Inline function given the token name for error messages */
const char *tokname(enum parse_tokid tokid)
{
  return tokinfo[tokid].name;
}

/* Search for the given string in the token list */
static inline int findkwd_cmp(const void *ap, const void *bp)
{
  const struct tokinfo *toka = (const struct tokinfo *)ap;
  const struct tokinfo *tokb = (const struct tokinfo *)bp;

  if (!toka->kwd || !tokb->kwd)
    return toka->id - tokb->id;
  return strcmp(toka->kwd, tokb->kwd);
}

enum parse_tokid findkwd(const char *text)
{
  if (text) {
    struct tokinfo kwdkey = { .kwd = text };
    struct tokinfo *kwdptr =
      bsearch(&kwdkey, tokinfo+TNOT, NUM_TOKEN-TNOT, sizeof(struct tokinfo),
          findkwd_cmp);
    if (kwdptr)
      return kwdptr->id;
  }
  return INV_TOKEN;
}

/* Declare the functions called by the main tokenizer function */
static bool int_readtoken(struct parse_context *ctx, struct parse_token *tok);

/* Provide the main tokenizer routine */
enum parse_tokid readtoken(struct parse_context *ctx)
{
  struct parse_token tok;
  struct parse_tokflags savekwd = ctx->chkflags;

  while (true) {
    /* Perform the reread logic here */
    if (ctx->tokpushback) {
      ctx->tokpushback = false;
      return ctx->last_token.id;
    }

    /* Call the internal function to actually retrieve the next token */
    int_readtoken(ctx, &tok);

    /* Eat newlines */
    if (savekwd.chknl) {
      while (tok.id == TNL) {
        parseheredoc(ctx);
        set_tokflags(&ctx->chkflags, tf_false, tf_false, tf_false, tf_keep);

        /* Perform the reread logic here */
        if (ctx->tokpushback) {
          ctx->tokpushback = false;
          return ctx->last_token.id;
        }

        /* Call the internal function to actually retrieve the next token */
        int_readtoken(ctx, &tok);
      }
    }

    mrg_tokflags(&savekwd, ctx->chkflags);
    set_tokflags(&ctx->chkflags, tf_false, tf_false, tf_false, tf_keep);

    if (tok.id != TWORD || ctx->quoteflag)
      return tok.id; 

    /* Check for keywords */
    if (savekwd.chkkwd) {
      enum parse_tokid kwd = findkwd(tok.val.text);
      if (kwd != INV_TOKEN) {
        tok.id = kwd;
        ctx->last_token = tok;
        return kwd;
      }
    }

    if (savekwd.chkalias) {
      /* TODO: Implement?? as not really executing the code */
    }
    return tok.id;
  }
}

/* Provide a file reader which will skip esacped newlines */
static inline char pgetc(struct parse_context *ctx)
{
  char chr;

  ctx->cur_char = chr = source_next_char(ctx);
  return chr;
}

static inline char pgetc_eatbnl(struct parse_context *ctx)
{
  char chr;

  while ((chr = source_next_char(ctx)) == '\\') {
    if (source_next_char(ctx) != '\n') {
      source_unget_char(ctx, chr);
      break;
    }
  }
  ctx->cur_char = chr;
  return chr;
}

static inline void pungetc(struct parse_context *ctx, char chr)
{
  source_unget_char(ctx, chr);
}

static bool syn_readtoken(struct parse_context *,
                          struct parse_token   *,
                          enum   parse_toksyn,
                          struct parse_heredoc *);

/* Declare the internal tokenizer function */
static bool int_readtoken(struct parse_context *ctx,
                          struct parse_token   *tok)
{
  if (!ctx || !tok) return false;
  
  /* Repeat checking until a token or word is found */
  while (true) {
    char chr = pgetc_eatbnl(ctx);
    switch (chr) {
      case ' ':
      case '\t':
        continue;

      case '#':
        while ((chr = pgetc(ctx)) != '\n' && chr != PEOF);
        pungetc(ctx, chr);
        continue;

      case '\n':
        tok->id = TNL;
        return true;

      case PEOF:
        tok->id = TEOF;
        return true;

      case '&':
        if (pgetc_eatbnl(ctx) == '&') {
          tok->id = TAND;
        } else {
          pungetc(ctx, chr);
          tok->id = TBACKGND;
        }
        return true;

      case '|':
        if (pgetc_eatbnl(ctx) == '|') {
          tok->id = TOR;
        } else {
          pungetc(ctx, chr);
          tok->id = TPIPE;
        } 
        return true;

      case ';':
        if (pgetc_eatbnl(ctx) == ';') {
          tok->id = TENDCASE;
        } else {
          pungetc(ctx, chr);
          tok->id = TSEMI;
        }
        return true;

      case '(':
        tok->id = TLP;
        return true;

      case ')':
        tok->id = TRP;
        return true;
    }
    return syn_readtoken(ctx, tok, SYN_BASE, NULL);
  }
}

static inline struct parse_syntax *
push_syntax(struct parse_context *ctx,
            enum parse_toksyn     type)
{
  struct parse_syntax node;
  if (!ctx->lst_syntax)
    ctx->lst_syntax = dtailq_init(ctx, NULL, sizeof(struct parse_syntax));
  node.type = type;
  node.varnest = 0;
  node.parenlevel = 0;
  node.dqvarnest = 0;
  node.innerdq = false;
  node.varpushed = false;
  node.dblquote = false;
  return dtailq_insert_head(ctx->lst_syntax, &node);
}

static inline struct parse_syntax *
pop_syntax(struct parse_context *ctx)
{
  return dtailq_remove_head(ctx->lst_syntax);
}

/* Return the tokenizer representation of the given character and syntax table */
enum parse_chrid {
  CWORD=0, CNL, CBACK, CSQUOTE, CDQUOTE, CENDQUOTE, CBQUOTE, CVAR,
  CENDVAR, CLP, CRP, CEOF, CCTL, CSPCL,
  CUNK                        /* Unknown token value */
};

static enum parse_chrid syn_lookup(struct parse_syntax *syn, char chr)
{
  enum parse_toksyn tsyn = syn ? syn->type : SYN_BASE;
  switch (chr) {
  case 0:
    return CEOF;

  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
    return CCTL;

  case '\n':
    switch (tsyn) {
    case SYN_BASE:
    case SYN_DQUOTE:
    case SYN_SQUOTE:
    case SYN_ARITH:
      return CNL;
    default:
      return CWORD;
    }
    break;

  case '\\':
    switch (tsyn) {
    case SYN_BASE:
    case SYN_DQUOTE:
    case SYN_ARITH:
      return CBACK;
    case SYN_SQUOTE:
      return CCTL;
    default:
      return CWORD;
    }
    break;

  case '\'':
    switch (tsyn) {
    case SYN_BASE:
      return CSQUOTE;
    case SYN_SQUOTE:
      return CENDQUOTE;
    default:
      return CWORD;
    }
    break;

  case '"':
    switch (tsyn) {
    case SYN_BASE:
      return CDQUOTE;
    case SYN_DQUOTE:
      return CENDQUOTE;
    default:
      return CWORD;
    }
    break;

  case '`':
    switch (tsyn) {
    case SYN_BASE:
    case SYN_DQUOTE:
    case SYN_ARITH:
      return CBQUOTE;
    default:
      return CWORD;
    }
    break;

  case '$':
    switch (tsyn) {
    case SYN_BASE:
    case SYN_DQUOTE:
    case SYN_ARITH:
      return CVAR;
    default:
      return CWORD;
    }
    break;

  case '}':
    switch (tsyn) {
    case SYN_BASE:
    case SYN_DQUOTE:
    case SYN_ARITH:
      return CENDVAR;
    default:
      return CWORD;
    }
    break;

  case '(':
    switch (tsyn) {
    case SYN_BASE:
      return CSPCL;
    case SYN_ARITH:
      return CLP;
    default:
      return CWORD;
    }
    break;

  case ')':
    switch (tsyn) {
    case SYN_BASE:
      return CSPCL;
    case SYN_ARITH:
      return CRP;
    default:
      return CWORD;
    }
    break;

  case '<':
  case '>':
  case ';':
  case '&':
  case '|':
  case ' ':
  case '\t':
    switch (tsyn) {
    case SYN_BASE:
      return CSPCL;
    default:
      return CWORD;
    }
    break;

  case '!':
  case '*':
  case '?':
  case '[':
  case '=':
  case '~':
  case ':':
  case '/':
  case '-':
  case ']':
    switch (tsyn) {
    case SYN_DQUOTE:
    case SYN_SQUOTE:
      return CCTL;
    default:
      return CWORD;
    }
    break;

  default:
    return CWORD;
  }
}

static void int_parseredir(struct parse_context *, char, char);
static void int_parsesub(struct parse_context *);
static void int_parsebackquote_old(struct parse_context *);
static void int_parsebackquote_new(struct parse_context *);

static bool syn_readtoken(struct parse_context *ctx,
                          struct parse_token   *tok,
                          enum   parse_toksyn   syntab,
                          struct parse_heredoc *heredoc)
{
  struct obstack *nctx = &ctx->memstack;
  struct obstack *sctx = &ctx->txtstack;
  struct parse_syntax *cursyn;
  char *txt;
  char chr = ctx->cur_char;
  bool loop_newline;

  /* Push the given syntax onto the stack */
  cursyn = push_syntax(ctx, syntab);
  ctx->quoteflag = false;
  stailq_clear(ctx->backquote);

  /* Keep processing lines until the end_of_word is flagged */
  do {
    bool end_of_word = false;     /* Set on end of word */
    loop_newline = false;         /* Set to redo loop */
    if (heredoc && heredoc->eofmark && heredoc->eofmark != FAKEEOFMARK) {
      char *ptr;
      bool nosave = false;

      if (heredoc->striptabs) {
        while (chr == '\t') 
          chr = pgetc(ctx);
      }

      for (ptr = heredoc->eofmark; obstack_1grow(sctx, chr), *ptr; ptr++) {
        if (chr != *ptr) {
          nosave = true;
          break;
        }
        chr = pgetc(ctx);
      }
      if (nosave) {
        obstack_free(sctx, obstack_finish(sctx));
      } else if (chr == '\n' || chr == PEOF) {
        chr = PEOF;
      } else {
        push_source(ctx, SRC_STRING, obstack_finish(sctx));
      }
    }

    while (!end_of_word) {
      switch (syn_lookup(cursyn, chr)) {
      case CNL: /* '\n' */
        if (cursyn->type == SYN_BASE && !cursyn->varnest) {
          end_of_word = true;
          break;
        }
        obstack_1grow(sctx, chr);
        if (cursyn->type == SYN_SQUOTE)
          chr = pgetc(ctx);
        else
          chr = pgetc_eatbnl(ctx);
        loop_newline = true;
        break;

      case CWORD:
        obstack_1grow(sctx, chr);
        break;

      case CCTL:
        if (!heredoc || cursyn->dblquote || cursyn->varnest)
          obstack_1grow(sctx, CTLESC);
        obstack_1grow(sctx, chr);
        break;

      case CBACK:
        chr = pgetc(ctx);
        if (chr == PEOF) {
          obstack_1grow(sctx, CTLESC);
          obstack_1grow(sctx, '\\');
          pungetc(ctx, chr);
        } else {
          if (cursyn->dblquote && chr != '\\' && chr != '`' && chr != '$' &&
              (chr != '"' || (heredoc && !cursyn->varnest)) &&
              (chr != '}' || !cursyn->varnest)) {
            obstack_1grow(sctx, CTLESC);
            obstack_1grow(sctx, '\\');
          }
          obstack_1grow(sctx, CTLESC);
          obstack_1grow(sctx, chr);
          ctx->quoteflag = true;
        }
        break;

      case CSQUOTE:
        cursyn->type = SYN_SQUOTE;
        if (!heredoc)
          obstack_1grow(sctx, CTLQUOTEMARK);
        break;

      case CDQUOTE:
        cursyn->type = SYN_DQUOTE;
        cursyn->dblquote = true;
        if (cursyn->varnest)
          cursyn->innerdq ^= true;
        if (!heredoc)
          obstack_1grow(sctx, CTLQUOTEMARK);
        break;

      case CENDQUOTE:
        if (heredoc && !cursyn->varnest) {
          obstack_1grow(sctx, chr);
          break;
        }
        if (!cursyn->dqvarnest) {
          cursyn->type = SYN_BASE;
          cursyn->dblquote = false;
        }
        ctx->quoteflag = true;
        if (chr == '"' && cursyn->varnest)
          cursyn->innerdq ^= true;
        if (!heredoc)
          obstack_1grow(sctx, CTLQUOTEMARK);
        break;

      case CVAR:
        int_parsesub(ctx);
        break;

      case CENDVAR:
        if (!cursyn->innerdq && cursyn->varnest) {
          if (!--cursyn->varnest && cursyn->varpushed)
            cursyn = pop_syntax(ctx);
          else if (cursyn->dqvarnest)
            cursyn->dqvarnest--;
          obstack_1grow(sctx, CTLENDVAR);
        } else {
          obstack_1grow(sctx, chr);
        }
        break;

      case CLP:
        cursyn->parenlevel++;
        obstack_1grow(sctx, chr);
        break;

      case CRP:
        if (cursyn->parenlevel > 0) {
          obstack_1grow(sctx, chr);
          --cursyn->parenlevel;
        } else {
          chr = pgetc_eatbnl(ctx);
          if (chr == ')') {
            obstack_1grow(sctx, CTLENDARI);
            cursyn = pop_syntax(ctx);
          } else {
            obstack_1grow(sctx, ')');
            pungetc(ctx, chr);
          }
        }
        break;

      case CBQUOTE:
        if (ctx->chkflags.chkeofmark) {
          obstack_1grow(sctx, '`');
        } else {
          int_parsebackquote_old(ctx);
        }
        break;

      case CEOF:
        end_of_word = true;
        break;

      default:
        if (cursyn->varnest) {
          obstack_1grow(sctx, chr);
        } else {
          end_of_word = true;
        }
        break;
      }
      if (!end_of_word) {
        if (cursyn->type == SYN_SQUOTE)
          chr = pgetc(ctx);
        else
          chr = pgetc_eatbnl(ctx);
      }
    }
  } while (loop_newline);

  if (cursyn->type == SYN_ARITH) {
    ctx_synerror(ctx, SE_MISSING, -1, "))"); goto fail;
  } else if (cursyn->type != SYN_BASE && !heredoc) {
    ctx_synerror(ctx, SE_QUOTESTR, -1, NULL); goto fail;
  } else if (cursyn->varnest) {
    ctx_synerror(ctx, SE_MISSING, -1, "}"); goto fail;
  }
  obstack_1grow(sctx, '\0');
  txt = obstack_finish(sctx);
  if (!heredoc) {
    if ((chr == '>' || chr == '<') && !ctx->quoteflag && 
        strlen(txt) <= 2 && (!*txt || isdigit(*txt))) {
      int_parseredir(ctx, chr, *txt);
      ctx->last_token.id = TREDIR;
    }
  } else {
    ctx->last_token.id = TWORD;
    ctx->last_token.val.text = txt;
  }
  return true;
fail:
  obstack_free(sctx, obstack_finish(sctx));
  obstack_free(nctx, obstack_finish(nctx));
  return false;
}

/* Process a here document */
void parseheredoc(struct parse_context *ctx)
{
  if (ctx->lst_heredoc) {
    struct parse_heredoc *hereptr;
    STAILQ_FOREACH(hereptr, ctx->lst_heredoc) {
      struct parse_token tok;
      enum parse_toksyn syntab;

      if (hereptr->here->type == NHERE) {
        ctx->cur_char = pgetc(ctx);
        syntab = SYN_SQUOTE;
      } else {
        ctx->cur_char = pgetc_eatbnl(ctx);
        syntab = SYN_DQUOTE;
      }
  
      if (!syn_readtoken(ctx, &tok, syntab, hereptr)) {
      }
    }
    stailq_clear(ctx->lst_heredoc);
  }
}

/* The following is the revised code found for PARSEREDIR */
static void int_parseredir(struct parse_context *ctx, char chr, char fd)
{
  union parse_node *np = node_alloc(ctx);

  switch (chr) {
  case '>':
    np->nfile.fd = 1;
    chr = pgetc_eatbnl(ctx);
    switch (chr) {
    case '>':
      np->type = NAPPEND;
      break;

    case '|':
      np->type = NCLOBBER;
      break;

    case '&':
      np->type = NTOFD;
      break;

    default:
      np->type = NTO;
      pungetc(ctx, chr);
    }
    break;

  case '<':
    chr = pgetc_eatbnl(ctx);
    switch (chr) {
    case '<':
      np->type = NHERE;
      np->nhere.fd = 0;
      chr = pgetc_eatbnl(ctx);
      ctx->cur_heredoc.here = np;
      if (!(ctx->cur_heredoc.striptabs = (chr == '-'))) {
        pungetc(ctx, chr);
      }
      break;

    case '&':
      np->type = NFROMFD;
      np->nfile.fd = 0;
      break;

    case '>':
      np->type = NFROMTO;
      np->nfile.fd = 0;
      break;

    default:
      np->type = NFROM;
      np->nfile.fd = 0;
      pungetc(ctx, chr);
    }
    if (isdigit(fd))
      np->nfile.fd = fd - '0';
  }
  memcpy(&ctx->cur_redir, np, sizeof(union parse_node));
}

/* The following is the revised code found for PARSESUB */
static void int_parsesub(struct parse_context *ctx)
{
  struct obstack *sctx = &ctx->txtstack;
  char chr;

  if (ctx->chkflags.chkeofmark) {
    obstack_1grow(sctx, '$');
    return;
  }
  chr = pgetc_eatbnl(ctx);
  if (chr == '(') {   /* $(command) or $((arith)) */
    chr = pgetc_eatbnl(ctx);
    if (chr == '(') {
      struct parse_syntax *cursyn = push_syntax(ctx, SYN_ARITH);
      cursyn->dblquote = true;
      obstack_1grow(sctx, CTLARI);
    } else {
      pungetc(ctx, chr);
      int_parsebackquote_new(ctx);
    }
  } else if (chr != '{' && !is_name(chr) && !is_special(chr)) {
    obstack_1grow(sctx, '$');
    pungetc(ctx, chr);
  } else {
    size_t typeloc;                  /* Offset to byte storing type info */
    enum parse_varsubs subtype;
    bool badsub = false;             /* Set if an invalid char */
    struct parse_syntax *cursyn = ctx->lst_syntax->first;
    enum parse_toksyn newsyn = cursyn->type;

    obstack_1grow(sctx, CTLVAR);
    typeloc = obstack_object_size(sctx);
    obstack_1grow(sctx, '\0');
    if (chr == '{') {
      chr = pgetc_eatbnl(ctx);
      subtype = VSNONE;
    } else {
      subtype = VSNORMAL;
    }
    while (true) {
      if (is_name(chr)) {
        do {
          obstack_1grow(sctx, chr);
          chr = pgetc_eatbnl(ctx);
        } while (is_in_name(chr));
      } else if (isdigit(chr)) {
        do {
          obstack_1grow(sctx, chr);
          chr = pgetc_eatbnl(ctx);
        } while ((subtype <= VSNONE || subtype >= VSLENGTH) && isdigit(chr));
      } else if (chr != '}') {
        char cc = chr;

        chr = pgetc_eatbnl(ctx);
        if (!subtype && cc == '#') {
          subtype = VSLENGTH;
          if (chr == '_' || isalnum(chr))
            continue;
          cc = chr;
          chr = pgetc_eatbnl(ctx);
          if (cc == '}' || chr == '}') {
            pungetc(ctx, chr);
            subtype = VSNONE;
            chr = cc;
            cc = '#';
          }
        }
        if (!is_special(cc)) {
          if (subtype == VSLENGTH)
            subtype = VSNONE;
          badsub = true;
          break;
        }
        obstack_1grow(sctx, cc);
      } else {
        badsub = true;
      }
      break;
    }
    if (badsub) {
      pungetc(ctx, chr);
    } else if (!subtype) {
      char cc = chr;
      switch (chr) {
      default:
        break;

      case ':':
        chr = pgetc_eatbnl(ctx);
        switch (chr) {
        default:
          subtype = VSNUL;
          break;

        case '}':
          subtype |= VSNORMAL;
          break;

        case '-':
          subtype |= VSMINUS;
          break;

        case '+':
          subtype |= VSPLUS;
          break;

        case '?':
          subtype |= VSQUESTION;
          break;

        case '=':
          subtype |= VSASSIGN;
          break;
        }
        break;

      case '}':
        subtype |= VSNORMAL;
        break;

      case '-':
        subtype |= VSMINUS; 
        break;

      case '+':
        subtype |= VSPLUS;
        break;

      case '?':
        subtype |= VSQUESTION;
        break;

      case '=':
        subtype |= VSASSIGN;
        break;

      case '%':
        chr = pgetc_eatbnl(ctx);
        if (chr == cc) {
          subtype = VSTRIMRIGHTMAX;
        } else {
          pungetc(ctx, chr);
          subtype = VSTRIMRIGHT;
        }
        newsyn = SYN_BASE;
        break;

      case '#':
        chr = pgetc_eatbnl(ctx);
        if (cc == chr) {
          subtype = VSTRIMLEFTMAX;
        } else {
          subtype = VSTRIMLEFT;
          pungetc(ctx, chr);
        }
        newsyn = SYN_BASE;
        break;
      }
    } else {
      if (subtype == VSLENGTH && chr != '}')
        subtype = VSNONE;
      pungetc(ctx, chr);
    }
    if (newsyn == SYN_ARITH)
      newsyn = SYN_DQUOTE;
    if ((newsyn != cursyn->type || cursyn->innerdq) && subtype != VSNORMAL) {
      cursyn = push_syntax(ctx, newsyn);
      cursyn->varpushed = true;
      cursyn->dblquote = newsyn != SYN_BASE;
    }
    obstack_1grow(sctx, '=');
    obstack_1grow(sctx, '\0');
    *(char *)(obstack_base(sctx) + typeloc) = VSBIT | subtype;
    if (subtype != VSNORMAL) {
      cursyn->varnest++;
      if (cursyn->dblquote)
        cursyn->dqvarnest++;
    }
  }
}

/* Parse an old backquote sequence, `...` */
static void int_parsebackquote_old(struct parse_context *ctx)
{
  struct parse_nodelist  newnode;
  struct obstack        *sctx = &ctx->txtstack;
  char chr;

  obstack_1grow(sctx, CTLBACKQ);
  do {
    chr = pgetc_eatbnl(ctx);
    switch (chr) {
    case '`':
      break;

    case '\\':
      chr = pgetc(ctx);
      if (chr != '\\' && chr != ' ' && chr != '$' && 
          ((*ctx->lst_syntax->last)->dblquote || chr != '"')) {
        obstack_1grow(sctx, '\\');
      }
      obstack_1grow(sctx, chr);
      break;

    case PEOF:
      ctx_synerror(ctx, SE_BACKEOF, -1, "EOF in backquote substitution");
      return;

    case '\n':
      obstack_1grow(sctx, chr);
      break;

    default:
      obstack_1grow(sctx, chr);
      break;
    }
  } while (chr != '`');
  obstack_1grow(sctx, '\0');
  push_source(ctx, SRC_STRING, obstack_finish(sctx));

  /* .. FIXME Complete implementation .. */
}

/* Parse a new backquote sequence, $(...) */
static void int_parsebackquote_new(struct parse_context *ctx)
{
}
