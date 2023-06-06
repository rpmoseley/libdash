/*
 * This header provides the definitions that are used by the parser based on those
 * used in the actual dash shell, these are used only by that parser and then they
 * are transposed into a form suitable for the generation of an AST.
 */
#pragma once

#include <ctype.h>
#include <obstack.h>
#include <stdbool.h>
#include <stdio.h>

/* Provide the various underlying parser nodes */
enum parse_nodetype {
  NCMD = 0, NPIPE, NREDIR, NBACKGND, NSUBSHELL, NAND, NOR, NSEMI, NIF, NWHILE,
  NUNTIL, NFOR, NCASE, NCLIST, NDEFUN, NARG, NTO, NCLOBBER, NFROM, NFROMTO,
  NAPPEND, NTOFD, NFROMFD, NHERE, NXHERE, NNOT, NEOF,
  NUM_PARSER_NODES
};

struct parse_ncmd {
  enum parse_nodetype type;
  unsigned int linno;
  union parse_node *assign;
  union parse_node *args;
  union parse_node *redirect;
};

struct parse_npipe {
  enum parse_nodetype type;
  bool backgnd;
  struct parse_nodelist *cmdlist;
};

struct parse_nredir {
  enum parse_nodetype type;
  unsigned int linno;
  union parse_node *node;
  union parse_node *redirect;
};

struct parse_nbinary {
  enum parse_nodetype type;
  union parse_node *ch1;
  union parse_node *ch2;
};

struct parse_nif {
  enum parse_nodetype type;
  union parse_node *test;
  union parse_node *ifpart;
  union parse_node *elsepart;
};

struct parse_nfor {
  enum parse_nodetype type;
  unsigned int linno;
  union parse_node *args;
  union parse_node *body;
  char *var;
};

struct parse_ncase {
  enum parse_nodetype type;
  unsigned int linno;
  union parse_node *expr;
  union parse_node *cases;
};

struct parse_nclist {
  enum parse_nodetype type;
  union parse_node *next;
  union parse_node *pattern;
  union parse_node *body;
};

struct parse_ndefun {
  enum parse_nodetype type;
  unsigned int linno;
  char *text;
  union parse_node *body;
};

struct parse_narg {
  enum parse_nodetype type;
  union parse_node *next;
  char *text;
  struct parse_nodelist *backquote;
};

struct parse_nfile {
  enum parse_nodetype type;
  union parse_node *next;
  int fd;
  union parse_node *fname;
  char *expfname;
};

struct parse_ndup {
  enum parse_nodetype type;
  union parse_node *next;
  int fd;
  int dupfd;
  union parse_node *vname;
};

struct parse_nhere {
  enum parse_nodetype type;
  union parse_node *next;
  int fd;
  union parse_node *doc;
};

struct parse_nnot {
  enum parse_nodetype type;
  union parse_node *com;
};

union parse_node {
  enum parse_nodetype type;
  struct parse_ncmd ncmd;
  struct parse_npipe npipe;
  struct parse_nredir nredir;
  struct parse_nbinary nbinary;
  struct parse_nif nif;
  struct parse_nfor nfor;
  struct parse_ncase ncase;
  struct parse_nclist nclist;
  struct parse_ndefun ndefun;
  struct parse_narg narg;
  struct parse_nfile nfile;
  struct parse_ndup ndup;
  struct parse_nhere nhere;
  struct parse_nnot nnot;
};

struct parse_nodelist {
  struct parse_nodelist *next;
  union parse_node *node;
};

#ifdef STAILQ_HEAD
STAILQ_HEAD(nodelist_hdr, nodelist);
#else
struct parse_nodelist_hdr {
  struct obstack memstack;
  struct parse_nodelist  *first;
  struct parse_nodelist **last;
  size_t sizelm;
};
#endif

struct parse_heredoc {
  struct parse_heredoc *next;
  union parse_node     *here;
  char                 *eofmark;
  bool                  striptabs;
};

#ifdef STAILQ_HEAD
STAILQ_HEAD(heredoc_hdr, heredoc);
#else
struct parse_heredoc_hdr {
  struct obstack memstack;
  struct parse_heredoc  *first;
  struct parse_heredoc **last;
  size_t sizelm;
};
#endif

/* Define the structure used to save the here document list when
 * processing a backquote command.
 */
struct parse_savheredoc {
  struct parse_savheredoc  *next;
  struct parse_heredoc_hdr  here;
};

enum parse_toksyn {
  SYN_BASE = 0,                   /* Normal syntax handling */
  SYN_SQUOTE,                     /* Single quote syntax handling */
  SYN_DQUOTE,                     /* Double quote syntax handling */
  SYN_BQUOTE,                     /* Back quote syntax handling */
  SYN_ARITH,                      /* Arithmetic handling */
};

struct parse_syntax {
  struct parse_syntax   *next;
  struct parse_syntax  **prev;
  enum parse_toksyn      type;
  unsigned int           varnest;
  unsigned int           parenlevel;
  unsigned int           dqvarnest;
  bool                   innerdq;
  bool                   varpushed;
  bool                   dblquote;
};

#ifdef DTAILQ_HEAD
DTAILQ_HEAD(syntax_hdr, syntax);
#else
struct parse_syntax_hdr {
  struct obstack memstack;
  struct parse_syntax  *first;
  struct parse_syntax **last;
  size_t sizelm;
};
#endif

enum parse_tokid {
  TEOF = 0, TNL, TSEMI, TBACKGND, TAND, TOR, TPIPE, TLP, TRP, TENDCASE,
  TENDBQUOTE, TREDIR, TWORD, TNOT, TCASE, TDO, TDONE, TELIF, TELSE, TESAC,
  TFI, TFOR, TIF, TIN, TTHEN, TUNTIL, TWHILE, TBEGIN, TEND,
  NUM_TOKEN,                        /* Should be last in enum */
  INV_TOKEN                         /* Used to represent an invalid token */
};

struct parse_token {
  enum parse_tokid    id;
  union {
    char             *text;
    union parse_node *node;
  } val;
};

enum parse_synerrcode {
  SE_NONE = 0,
  SE_UNKNOWN,
  SE_EXPECTED,
  SE_BADFORVAR,
  SE_BADFUNCNAME,
  SE_BADFDNUM,
  SE_MISSING,
  SE_QUOTESTR,
  SE_BACKEOF,
};

struct parse_synerror {
  enum parse_synerrcode  code;
  struct parse_token     token;
  char                  *errtext;
};

struct parse_tokflags {
  bool chkalias   : 1,
       chkkwd     : 1,
       chknl      : 1,
       chkeofmark : 1,
       chkendtok  : 1;
};

enum tri_value {
  tf_false = 0,
  tf_true,
  tf_keep
};

static inline void clr_tokflags(struct parse_tokflags *flag)
{
  memset(flag, 0, sizeof(struct parse_tokflags));
}

static inline void set_tokflags(struct parse_tokflags *flag,
                                enum tri_value         chkalias,
                                enum tri_value         chkkwd,
                                enum tri_value         chknl,
                                enum tri_value         chkeofmark)
{
#define SET_TOKFLAG(flag, fld) \
  flag->fld = (fld == tf_keep ? flag->fld : (fld == tf_true ? true : false))
  SET_TOKFLAG(flag, chkalias);
  SET_TOKFLAG(flag, chkkwd);
  SET_TOKFLAG(flag, chknl);
  SET_TOKFLAG(flag, chkeofmark);
#undef SET_TOKFLAG
}

#define SET_IND_TOKFLAG(fld) \
static inline void set_tokflags_##fld(struct parse_tokflags *flag, bool val) \
{ flag->fld = val; }
SET_IND_TOKFLAG(chkalias)
SET_IND_TOKFLAG(chkkwd)
SET_IND_TOKFLAG(chknl)
SET_IND_TOKFLAG(chkeofmark)
SET_IND_TOKFLAG(chkendtok)
#undef SET_IND_TOKFLAG

static inline void mrg_tokflags(struct parse_tokflags *flag,
                                struct parse_tokflags  mflag)
{
  flag->chkalias   |= mflag.chkalias;
  flag->chkkwd     |= mflag.chkkwd;
  flag->chknl      |= mflag.chknl;
  flag->chkeofmark |= mflag.chkeofmark;
}
static inline bool any_tokflags(struct parse_tokflags flag)
{
  union {
    bool                  all;
    struct parse_tokflags flg;
  } chkflag = { .flg = flag };
  return !!chkflag.all;
}

enum parse_interrcode {
  IE_NONE = 0,                             /* No internal error */
  IE_NOSOURCE,                             /* No source stack defined */
  IE_NOUNGET,                              /* No data to unget */
  IE_NOGETCHR,                             /* No data to get */
};

/* Define the types of source that can be processed */
enum parse_srctype {
  SRC_FILE = 0,                /* Source is a named file */
  SRC_STRING,                  /* Source is a string */
  SRC_NUM,                     /* Number of source types */
#if defined(NEED_DUMMY_OPS) && NEED_DUMMY_OPS
  SRC_DUMMY                    /* Dummy source */
#endif
};

struct parse_source;
#ifdef STAILQ_HEAD
STAILQ_HEAD(source_hdr, source);
#else
struct parse_source_hdr {
  struct obstack        memstack;
  struct parse_source  *first;
  struct parse_source **last;
  size_t sizelm;
};
#endif

struct parse_context {
  struct obstack             memstack;       /* Used for node allocation */
  struct obstack             txtstack;       /* Used for textual values */
  struct parse_source_hdr   *lst_source;     /* Queue of sources */
  struct parse_syntax_hdr   *lst_syntax;     /* Queue of syntax tables */
  struct parse_heredoc_hdr  *lst_heredoc;    /* Queue of here documents */
  struct parse_savheredoc   *sav_heredoc;    /* List of saved here documents */
  union  parse_node          cur_redir;      /* Current redirection */
  struct parse_heredoc       cur_heredoc;    /* Current here document */
  struct parse_nodelist_hdr *backquote;      /* Current backquote commands */
  struct parse_token         last_token;     /* Last token returned */
  struct parse_tokflags      chkflags;       /* Current parser flags */
  struct parse_synerror      synerror;       /* Syntax error status */
  enum   parse_interrcode    int_error;      /* Internal error status */
  char                       cur_char;       /* Last character read */
  char                       lst_char[2];    /* Previous characters */
  bool                       tokpushback;    /* Set if token pushed back */
  bool                       quoteflag;      /* Set if in quote mode */
};

#define FAKEEOFMARK (const char *)1

/* Control characters in argument strings */
enum parse_esccodes {
  PEOF = 0, CTLESC, CTLVAR, CTLENDVAR, CTLBACKQ, CTLARI, CTLENDARI, CTLQUOTEMARK
};

enum parse_varsubs {
  VSTYPE         = 0xf,         /* Type of variable substitution */
  VSNUL          = 0x10,        /* Colon--treat the empty string as unset */
  VSBIT          = 0x20,        /* Ensure subtype is not zero */
  VSNONE         = 0,           /* No special value */
  VSNORMAL       = 0x1,         /* Normal variable: $var or ${var} */
  VSMINUS        = 0x2,         /* ${var-text} */
  VSPLUS         = 0x3,         /* ${var+text} */
  VSQUESTION     = 0x4,         /* ${var?message} */
  VSASSIGN       = 0x5,         /* ${var=text} */
  VSTRIMRIGHT    = 0x6,         /* ${var%pattern} */
  VSTRIMRIGHTMAX = 0x7,         /* ${var%%pattern} */
  VSTRIMLEFT     = 0x8,         /* ${var#pattern} */
  VSTRIMLEFTMAX  = 0x9,         /* ${var##pattern} */
  VSLENGTH       = 0xa,         /* ${#var} */
};

#ifndef __CONCAT
# define __CONCAT(a, b)  a ## b
#endif
#define _SUNAME(su, type)    su __CONCAT(parse_, type)
#define NODEALLOC(su, type)                                                       \
static inline union parse_node *__CONCAT(type, _alloc)(struct parse_context *ctx) \
{                                                                                 \
  return (union parse_node *)obstack_alloc(&ctx->memstack,                        \
                               sizeof(_SUNAME(su, type)));                        \
}                                                                                 \
static inline union parse_node *__CONCAT(type, _copy)(struct parse_context *ctx,  \
                               _SUNAME(su, type) *ptr)                            \
{                                                                                 \
  return (union parse_node *)obstack_copy(&ctx->memstack, ptr,                    \
                               sizeof(_SUNAME(su, type)));                        \
}                                                                                 \
static inline void __CONCAT(type, _free)(struct parse_context *ctx,               \
                               _SUNAME(su, type) *ptr)                            \
{                                                                                 \
  obstack_free(&ctx->memstack, ptr);                                              \
}
NODEALLOC(union,  node)
NODEALLOC(struct, nredir)
NODEALLOC(struct, nbinary)
NODEALLOC(struct, npipe)
NODEALLOC(struct, nnot)
NODEALLOC(struct, nif)
NODEALLOC(struct, nfor)
NODEALLOC(struct, narg)
NODEALLOC(struct, ncase)
NODEALLOC(struct, nclist)
NODEALLOC(struct, ncmd)

static inline struct parse_nodelist *nodelist_alloc(struct parse_context *ctx)
{
  struct parse_nodelist *node = obstack_alloc(&ctx->memstack,
		  sizeof(struct parse_nodelist));
  node->next = NULL;
  node->node = NULL;
  return node;
}
static inline struct parse_nodelist *nodelist_copy(struct parse_context *ctx,
                                                   struct parse_nodelist *node)
{
  return obstack_copy(&ctx->memstack, node, sizeof(struct parse_nodelist));
}
static inline void nodelist_free(struct parse_context *ctx,
                                 struct parse_nodelist *node)
{
  if (node)
    obstack_free(&ctx->memstack, node);
}

/* Provide definitions of opaque structures */
struct builtincmd;

static inline bool is_name(char chr) {
  return chr == '_' || isalpha(chr);
}

static inline bool is_in_name(char chr) {
  return chr == '_' || isalnum(chr);
}

static inline bool is_special(char chr) {
  return isdigit(chr) || strchr("#?$!-*@", chr);
}

static inline void push_heredoclist(struct parse_context *ctx)
{
  struct parse_savheredoc *newp = obstack_alloc(&ctx->memstack,
        sizeof(struct parse_savheredoc));
  memcpy(&newp->here, &ctx->lst_heredoc, sizeof(struct parse_heredoc_hdr));
  newp->next = ctx->sav_heredoc;
  ctx->sav_heredoc = newp;
}

static inline void pop_heredoclist(struct parse_context *ctx)
{
  struct parse_savheredoc *frep = ctx->sav_heredoc;
  if (frep) {
    ctx->sav_heredoc = frep->next;
    memcpy(&ctx->lst_heredoc, frep, sizeof(struct parse_heredoc_hdr));
    obstack_free(&ctx->memstack, frep);
  } else {
    ctx->sav_heredoc = frep;
  }
}

/* Provide all the external symbols provided by the various sources */
extern union parse_node *list_et(struct parse_context *);
extern union parse_node *parse_next_command(struct parse_context *);
extern enum parse_tokid readtoken(struct parse_context *);
extern bool endtoklist(enum parse_tokid);
extern void parseheredoc(struct parse_context *);
extern void ctx_synerror_expect(struct parse_context *, enum parse_tokid);
extern void ctx_synerror(struct parse_context *, enum parse_synerrcode,
    enum parse_tokid, char *);
extern bool ctx_init(struct parse_context **);
extern void ctx_fini(struct parse_context **);
extern union parse_node *ctx_next_command(struct parse_context *);
extern unsigned int source_currline(struct parse_context *);
extern const struct builtincmd *find_builtin(const char *);
extern bool builtin_isspecial(const struct builtincmd *);
extern char source_next_char(struct parse_context *);
extern void source_unget_char(struct parse_context *,char);
extern int init_source(struct parse_context *);
extern int fini_source(struct parse_context *);
extern struct parse_source *push_source(struct parse_context *,
    enum parse_srctype, void const *);
extern struct parse_source *pop_source(struct parse_context *);
extern union parse_node *eof_node(void);
