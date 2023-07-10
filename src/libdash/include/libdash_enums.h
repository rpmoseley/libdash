/*
 * This header provides the public enums that are defined and used by the
 * libdash library, this avoids having multiple definitions which require
 * to be maintained.
 */

#pragma once

/* Define the types of node that can be handled by the library */
enum parse_nodetype {
  NCMD = 0, NPIPE, NREDIR, NBACKGND, NSUBSHELL, NAND, NOR, NSEMI, NIF, NWHILE,
  NUNTIL, NFOR, NCASE, NCLIST, NDEFUN, NARG, NTO, NCLOBBER, NFROM, NFROMTO,
  NAPPEND, NTOFD, NFROMFD, NHERE, NXHERE, NNOT, NEOF,
  NUM_PARSER_NODES,
  INVALID_NODE
};

