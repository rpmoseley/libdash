/*
 * This header provides the enums that are defined and used by the parser
 */

#pragma once

/* Define the types of node that can be handled by the library */
enum parse_nodetype {
  NCMD = 0, NPIPE, NREDIR, NBACKGND, NSUBSHELL, NAND, NOR, NSEMI, NIF, NWHILE,
  NUNTIL, NFOR, NCASE, NCLIST, NDEFUN, NARG, NTO, NCLOBBER, NFROM, NFROMTO,
  NAPPEND, NTOFD, NFROMFD, NHERE, NXHERE, NNOT, NEOF,
  NUM_PARSER_NODES,                 /* Number of types of node */
  INV_PARSER_NODE                   /* Represents an invalid node */
};

/* Define the types of token that can be handled by the library */
enum parse_tokid {
  TEOF = 0, TNL, TSEMI, TBACKGND, TAND, TOR, TPIPE, TLP, TRP, TENDCASE,
  TENDBQUOTE, TREDIR, TWORD, TNOT, TCASE, TDO, TDONE, TELIF, TELSE, TESAC,
  TFI, TFOR, TIF, TIN, TTHEN, TUNTIL, TWHILE, TBEGIN, TEND,
  NUM_PARSER_TOKEN,                 /* Number of types of token */
  INV_PARSER_TOKEN                  /* Represents an invalid token */
};

enum parse_toksyn {
  SYN_BASE = 0,                   /* Normal syntax handling */
  SYN_SQUOTE,                     /* Single quote syntax handling */
  SYN_DQUOTE,                     /* Double quote syntax handling */
  SYN_BQUOTE,                     /* Back quote syntax handling */
  SYN_ARITH,                      /* Arithmetic handling */
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

enum tri_value {
  tf_false = 0,
  tf_true,
  tf_keep
};

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
