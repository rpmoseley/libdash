/*
 * This is the parser that is based on the dash parser with some changes to enable
 * it to be used multiple times with different contexts. It's purpose is to output
 * each command of a script at a time, which will then be converted to a format
 * suitable for conversion into an AST.
 */

#include "include/parser.h"
#include "include/queue.h"

static inline bool goodname(const char *word)
{
  const char *p = word;
  if (is_name(*p))
    while (*++p && is_in_name(*p));
  return !*p;
}

static inline bool isassignment(const char *word)
{
  const char *p = word;
  if (is_name(*p))
    while (*++p && is_in_name(*p));
  return p != word && *p == '=';
}

/* Provide forward definitions of the parser functions */
static union parse_node *list(struct parse_context *ctx);
static union parse_node *andor(struct parse_context *ctx);
static union parse_node *pipeline(struct parse_context *ctx);
static union parse_node *command(struct parse_context *ctx);
static union parse_node *simplecmd(struct parse_context *ctx);

static inline union parse_node *list_nl(struct parse_context *ctx)
{
  ctx->chkflags.chknl = true;
  ctx->chkflags.chkendtok = false;
  return list(ctx);
}
union parse_node *list_et(struct parse_context *ctx)
{
  ctx->chkflags.chknl = true;
  ctx->chkflags.chkendtok = true;
  return list(ctx);
}

/* Provide a unique node that represents the end of input */
static union parse_node _eof_node = { .type = NEOF };
union parse_node *eof_node(void) {
  return &_eof_node;
}

/* Main entry point for the parser. Read and parse a single command, returns
 * NEOF on end of file, unlike the original parser it will skip empty lines
 */
union parse_node *ctx_next_command(struct parse_context *ctx)
{
  union parse_node *nxt_node;
  
  if (!ctx) return NULL;
  ctx->tokpushback = false;
  stailq_clear(ctx->lst_heredoc);
  do {
    ctx->chkflags.chknl = false;
    ctx->chkflags.chkendtok = false;
  } while (!(nxt_node = list(ctx)) && ctx->int_error == IE_NONE);
  return nxt_node;
}

static union parse_node *list(struct parse_context *ctx)
{
  union parse_node *n1 = NULL, *n2, *n3;
  enum parse_tokid tok;

  while (true) {
    set_tokflags(&ctx->chkflags, tf_true, tf_true, tf_keep, tf_keep);
    tok = readtoken(ctx);
    switch (tok) {
    case TNL:
      parseheredoc(ctx);
      return n1;

    case TEOF:
      if (!n1 && !ctx->chkflags.chknl)
          n1 = eof_node();
      parseheredoc(ctx);
      ctx->tokpushback = true;
      ctx->last_token.id = TEOF;
      return n1;

    default:
      break;
    }
    ctx->tokpushback = true;
    if (ctx->chkflags.chkendtok && endtoklist(tok))
      return n1;
    else
      ctx->chkflags.chkendtok = ctx->chkflags.chknl;

    n2 = andor(ctx);
    tok = readtoken(ctx);
    if (tok == TBACKGND) {
      if (n2->type == NPIPE) {
        n2->npipe.backgnd = true;
      } else {
        if (n2->type != NREDIR) {
          n3 = nredir_alloc(ctx);
          n3->nredir.node = n2;
          n3->nredir.redirect = NULL;
          n2 = n3;
        }
        n2->type = NBACKGND;
      }
    }
    if (!n1) {
      n1 = n2;
    } else {
      n3 = nbinary_alloc(ctx);
      n3->type = NSEMI;
      n3->nbinary.ch1 = n1;
      n3->nbinary.ch2 = n2;
      n1 = n3;
    }
    switch (tok) {
    case TEOF:
      parseheredoc(ctx);
      ctx->tokpushback = true;
      ctx->last_token.id = TEOF;
      return n1;

    case TNL:
      ctx->tokpushback = true;
    case TBACKGND:
    case TSEMI:
      break;

    default:
      if (!ctx->chkflags.chknl) {
        ctx_synerror_expect(ctx, -1);
        return NULL;
      }
      ctx->tokpushback = true;
      return n1;
    }
  }
}

static union parse_node *andor(struct parse_context *ctx)
{
  union parse_node *n1, *n2, *n3;
  enum parse_tokid tok;
  enum parse_nodetype toknode;

  n1 = pipeline(ctx);
  while (true) {
    if ((tok = readtoken(ctx)) == TAND) {
      toknode = NAND;
    } else if (tok == TOR) {
      toknode = NOR;
    } else {
      ctx->tokpushback = true;
      return n1;
    }
    set_tokflags(&ctx->chkflags, tf_true, tf_true, tf_true, tf_keep);
    n2 = pipeline(ctx);
    n3 = nbinary_alloc(ctx);
    n3->type = toknode;
    n3->nbinary.ch1 = n1;
    n3->nbinary.ch2 = n2;
    n1 = n3;
  }
}

static union parse_node *pipeline(struct parse_context *ctx)
{
  union parse_node *n1, *n2, *pipenode;
  struct parse_nodelist *lp;
  bool negate = false;

  if (readtoken(ctx) == TNOT) {
    negate = !negate;
    set_tokflags(&ctx->chkflags, tf_true, tf_true, tf_false, tf_keep);
  } else {
    ctx->tokpushback = true;
  }
  n1 = command(ctx);
  if (readtoken(ctx) == TPIPE) {
    pipenode = npipe_alloc(ctx);
    pipenode->type = NPIPE;
    pipenode->npipe.backgnd = false;
    lp = nodelist_alloc(ctx);
    pipenode->npipe.cmdlist = lp;
    lp->node = n1;
    do {
      struct parse_nodelist *prev;

      prev = lp;
      lp = nodelist_alloc(ctx);
      lp->node = command(ctx);
      prev->next = lp;
    } while (readtoken(ctx) == TPIPE);
    lp->next = NULL;
    n1 = pipenode;
  }
  ctx->tokpushback = true;
  if (negate) {
    n2 = nnot_alloc(ctx);
    n2->type = NNOT;
    n2->nnot.com = n1;
    return n2;
  } else {
    return n1;
  }
}

static inline char *tok_strdup(struct parse_context *ctx)
{
  return obstack_copy0(&ctx->memstack, ctx->last_token.val.text,
      strlen(ctx->last_token.val.text));
}

/* Behaves like original expand.c:rmescapes(s, 0) */
static char *rmescapes(char *str)
{
  char *p, *q;
  char cqchars[] = {
#ifdef HAVE_FNMATCH
    '^',
#endif
    CTLQUOTEMARK,
    CTLESC,
    '\0'
  };

  if (!(p = strpbrk(str, cqchars)))
    return str;
  q = p;
  while (*p) {
    if (*p == CTLQUOTEMARK) {
      p++;
      continue;
    }
    if (*p == '\\') {
      *q++ = *p++;
      continue;
    }
#ifdef HAVE_FNMATCH
    if (*p == '^') {
      *q++ = *p++;
      continue;
    }
#endif
    if (*p == CTLESC)
      p++;
    *q++ = *p++;
  }
  *q = '\0';
  return str;
}

static void parsefname(struct parse_context *ctx)
{
  union parse_node *n = &ctx->cur_redir;

  if (n->type == NHERE) {
    struct parse_heredoc *here;

    set_tokflags(&ctx->chkflags, tf_false, tf_false, tf_false, tf_true);
    if (readtoken(ctx) != TWORD) {
      ctx_synerror_expect(ctx, -1);
      return;
    }
    if (!ctx->lst_heredoc) {
      ctx->lst_heredoc = obstack_alloc(&ctx->memstack,
          sizeof(struct parse_heredoc_hdr));
    }
    here = obstack_copy(&ctx->lst_heredoc->memstack, &ctx->cur_heredoc,
        sizeof(struct parse_heredoc));
    if (!ctx->quoteflag)
      n->type = NXHERE;
    here->eofmark = tok_strdup(ctx);
    rmescapes(here->eofmark);
    stailq_insert_tail(ctx->lst_heredoc, here);    
  } else if (n->type == NTOFD || n->type == NFROMFD) {
    char *text = ctx->last_token.val.text;

    if (isdigit(text[0]) && text[1] == '\0') {
      n->ndup.dupfd = text[0] - '0';
    } else if (text[0] == '-' && text[1] == '\0') {
      n->ndup.dupfd = -1;
    } else {
      union parse_node *newn;

      newn = obstack_alloc(&ctx->memstack, sizeof(union parse_node));
      newn->type = NARG;
      newn->narg.next = NULL;
      newn->narg.text = tok_strdup(ctx);
      stailq_copy(&newn->narg.backquote, ctx->backquote);
      n->ndup.vname = newn;
    }
  }
}

static union parse_node *command(struct parse_context *ctx)
{
  union parse_node *n1, *n2;
  union parse_node *ap, **app;
  union parse_node *cp, **cpp;
  union parse_node *redir = NULL, **rpp, **rpp2 = &redir;
  enum parse_tokid tok;
  unsigned int savelinno = source_currline(ctx);
  bool skip_check = false;

  switch (readtoken(ctx)) {
  default:
    ctx_synerror_expect(ctx, -1);
    return NULL;

  case TIF:
    n1 = nif_alloc(ctx);
    n1->type = NIF;
    n1->nif.test = list_nl(ctx);
    if (readtoken(ctx) != TTHEN) {
      ctx_synerror_expect(ctx, TTHEN);
      return NULL;
    }
    n1->nif.ifpart = list_nl(ctx);
    n2 = n1;
    while (readtoken(ctx) == TELIF) {
      n2->nif.elsepart = nif_alloc(ctx);
      n2 = n2->nif.elsepart;
      n2->type = NIF;
      n2->nif.test = list_nl(ctx);
      if (readtoken(ctx) != TTHEN) {
        ctx_synerror_expect(ctx, TTHEN);
        return NULL;
      }
      n2->nif.ifpart = list_nl(ctx);
    }
    if (ctx->last_token.id == TELSE) {
      n2->nif.elsepart = list_nl(ctx);
    } else {
      n2->nif.elsepart = NULL;
      ctx->tokpushback = true;
    }
    tok = TFI;
    break;

  case TWHILE:
  case TUNTIL:
    n1 = nbinary_alloc(ctx);
    n1->type = (ctx->last_token.id == TWHILE ? NWHILE : NUNTIL);
    n1->nbinary.ch1 = list_nl(ctx);
    if (readtoken(ctx) != TDO) {
      ctx_synerror_expect(ctx, TDO);
      return NULL;
    }
    n1->nbinary.ch2 = list_nl(ctx);
    tok = TDONE;
    break;

  case TFOR:
    if (readtoken(ctx) != TWORD ||
        ctx->quoteflag ||
        !goodname(ctx->last_token.val.text)) {
      ctx_synerror(ctx, SE_BADFORVAR, -1, NULL);
      return NULL;
    }
    n1 = nfor_alloc(ctx);
    n1->type = NFOR;
    n1->nfor.linno = savelinno;
    n1->nfor.var = tok_strdup(ctx);
    set_tokflags(&ctx->chkflags, tf_true, tf_true, tf_true, tf_keep);
    if (readtoken(ctx) == TIN) {
      app = &ap;
      while (readtoken(ctx) == TWORD) {
        n2 = narg_alloc(ctx);
        n2->type = NARG;
        n2->narg.text = tok_strdup(ctx);
        stailq_copy(&n2->narg.backquote, ctx->backquote);
        *app = n2;
        app = &n2->narg.next;
      }
      *app = NULL;
      n1->nfor.args = ap;
      if (ctx->last_token.id != TNL && ctx->last_token.id != TSEMI) {
        ctx_synerror_expect(ctx, ctx->last_token.id);
      }
    } else {
      char dolatstr[] = { CTLQUOTEMARK, CTLVAR, VSNORMAL | VSBIT, '@', '=',
                          CTLQUOTEMARK };
      n2 = narg_alloc(ctx);
      n2->type = NARG;
      obstack_copy0(&ctx->memstack, dolatstr, 7);
      n2->narg.text = obstack_finish(&ctx->memstack);
      n2->narg.backquote = NULL;
      n2->narg.next = NULL;
      n1->nfor.args = n2;
      if (ctx->last_token.id != TSEMI)
        ctx->tokpushback = true;
    }
    set_tokflags(&ctx->chkflags, tf_true, tf_true, tf_true, tf_keep);
    if (readtoken(ctx) != TDO) {
      ctx_synerror_expect(ctx, TDO);
      return NULL;
    }
    n1->nfor.body = list_nl(ctx);
    tok = TDONE;
    break;

  case TCASE:
    n1 = ncase_alloc(ctx);
    n1->type = NCASE;
    n1->ncase.linno = savelinno;
    if (readtoken(ctx) != TWORD) {
      ctx_synerror_expect(ctx, TWORD);
      return NULL;
    }
    n1->ncase.expr = n2 = narg_alloc(ctx);
    n2->type = NARG;
    n2->narg.text = tok_strdup(ctx);
    stailq_copy(&n2->narg.backquote, ctx->backquote);
    n2->narg.next = NULL;
    set_tokflags(&ctx->chkflags, tf_true, tf_true, tf_true, tf_keep);
    if (readtoken(ctx) != TIN) {
      ctx_synerror_expect(ctx, TIN);
      return NULL;
    }
    cpp = &n1->ncase.cases;
    set_tokflags(&ctx->chkflags, tf_false, tf_true, tf_true, tf_keep);
    tok = readtoken(ctx);
    while (tok != TESAC) {
      if (ctx->last_token.id == TLP)
        readtoken(ctx);
      *cpp = cp = nclist_alloc(ctx);
      while (true) {
        *app = ap = narg_alloc(ctx);
        ap->type = NARG;
        ap->narg.text = tok_strdup(ctx);
        stailq_copy(&ap->narg.backquote, ctx->backquote);
        if (readtoken(ctx) != TPIPE)
          break;
        app = &ap->narg.next;
        readtoken(ctx);
      }
      ap->narg.next = NULL;
      if (ctx->last_token.id != TRP) {
        ctx_synerror_expect(ctx, TRP);
        return NULL;
      }
      cp->nclist.body = list_et(ctx);
      cpp = &cp->nclist.next;
      set_tokflags(&ctx->chkflags, tf_false, tf_true, tf_true, tf_keep);
      if ((tok = readtoken(ctx)) != TESAC) {
        if (tok != TENDCASE) {
          ctx_synerror_expect(ctx, TENDCASE);
          return NULL;
        } else {
          set_tokflags(&ctx->chkflags, tf_false, tf_true, tf_true, tf_keep);
          tok = readtoken(ctx);
        }
      }
    }
    *cpp = NULL;
    skip_check = true;
    break;

  case TLP:
    n1 = nredir_alloc(ctx);
    n1->type = NSUBSHELL;
    n1->nredir.linno = savelinno;
    n1->nredir.node = list_nl(ctx);
    n1->nredir.redirect = NULL;
    tok = TRP;
    break;

  case TBEGIN:
    n1 = list_nl(ctx);
    tok = TEND;
    break;

  case TWORD:
  case TREDIR:
    ctx->tokpushback = true;
    return simplecmd(ctx);
  }

  if (!skip_check && readtoken(ctx) != tok) {
    ctx_synerror_expect(ctx, tok);
    return NULL;
  }

  /* Check for redirections */
  set_tokflags(&ctx->chkflags, tf_true, tf_true, tf_false, tf_keep);
  rpp = rpp2;
  while (readtoken(ctx) == TREDIR) {
    *rpp = n2 = node_copy(ctx, &ctx->cur_redir);
    rpp = &n2->nfile.next;
    parsefname(ctx);
  }

  ctx->tokpushback = true;
  *rpp = NULL;
  if (redir) {
    if (n1->type != NSUBSHELL) {
      n2 = nredir_alloc(ctx);
      n2->type = NREDIR;
      n2->nredir.linno = savelinno;
      n2->nredir.node = n1;
      n1 = n2;
    }
    n1->nredir.redirect = redir;
  }
  return n1;
}

static union parse_node *simplecmd(struct parse_context *ctx)
{
  union parse_node *args = NULL, **app = &args;
  union parse_node *node = NULL;
  union parse_node *vars = NULL, **vpp = &vars;
  union parse_node *redir = NULL, **rpp = &redir;
  unsigned int savelinno = source_currline(ctx);
  struct parse_tokflags saveflags;

  set_tokflags(&saveflags, tf_true, tf_false, tf_false, tf_false);
  while (true) {
    ctx->chkflags = saveflags;
    switch (readtoken(ctx)) {
    case TWORD:
      node = narg_alloc(ctx);
      node->type = NARG;
      node->narg.text = tok_strdup(ctx);
      stailq_copy(&node->narg.backquote, ctx->backquote);
      if (any_tokflags(saveflags) && isassignment(ctx->last_token.val.text)) {
        *vpp = node;
        vpp = &node->narg.next;
      } else {
        *app = node;
        app = &node->narg.next;
        clr_tokflags(&saveflags);
      }
      break;

    case TREDIR:
      *rpp = node = node_copy(ctx, &ctx->cur_redir);
      rpp = &node->nfile.next;
      parsefname(ctx);
      break;

    case TLP:
      if (args && app == &args->narg.next && !vars && !redir) {
        const struct builtincmd *bcmd;
        const char *name;

        /* We have a function */
        if (readtoken(ctx) != TRP) {
          ctx_synerror_expect(ctx, TRP);
          return NULL;
        }
        name = tok_strdup(ctx);
        if (!goodname(name) || ((bcmd = find_builtin(name)) && builtin_isspecial(bcmd))) {
          ctx_synerror(ctx, SE_BADFUNCNAME, -1, NULL);
          return NULL;
        }
        node->type = NDEFUN;
        set_tokflags(&ctx->chkflags, tf_true, tf_true, tf_true, tf_keep);
        node->ndefun.text = node->narg.text;
        node->ndefun.linno = source_currline(ctx);
        node->ndefun.body = command(ctx);
        return node;
      }

    default:
      ctx->tokpushback = true;
      goto out;
    }
  }
out:
  *app = NULL;
  *vpp = NULL;
  *rpp = NULL;
  node = ncmd_alloc(ctx);
  node->type = NCMD;
  node->ncmd.linno = savelinno;
  node->ncmd.args = args;
  node->ncmd.assign = vars;
  node->ncmd.redirect = redir;
  return node;
}
