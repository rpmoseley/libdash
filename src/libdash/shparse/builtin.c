/*
 * This source provides the list of builtin commands
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <strings.h>

struct builtincmd {
  const char *name;                   /* Name of the builtin command */
  unsigned short flags;               /* Flags for the builtin command */
};

static const struct builtincmd builtin[] = {
  { ".",        3 },
  { ":",        3 },
  { "[",        0 },
  { "alias",    6 },
  { "bg",       2 },
  { "break",    3 },
  { "cd",       2 },
  { "chdir",    0 },
  { "command",  2 },
  { "continue", 3 },
  { "echo",     0 },
  { "eval",     3 },
  { "exec",     3 },
  { "exit",     3 },
  { "export",   7 },
  { "false",    2 },
  { "fg",       2 },
  { "getopts",  2 },
  { "hash",     2 },
  { "jobs",     2 },
  { "kill",     2 },
  { "local",    7 },
  { "printf",   0 },
  { "pwd",      2 },
  { "read",     2 },
  { "readonly", 7 },
  { "return",   3 },
  { "set",      3 },
  { "shift",    3 },
  { "test",     0 },
  { "times",    3 },
  { "trap",     3 },
  { "true",     2 },
  { "type",     2 },
  { "ulimit",   2 },
  { "umask",    2 },
  { "unalias",  2 },
  { "unset",    3 },
  { "wait",     2 },
};
#define NUM_BLTIN (sizeof(builtin) / sizeof(struct builtincmd))

static inline int
bltin_compare(const void *key, const void *elem)
{
  const struct builtincmd *bkey = (const struct builtincmd *)key;
  const struct builtincmd *belem = (const struct builtincmd *)elem;

  return strcasecmp(bkey->name, belem->name);
}

/* Check if given name is a builtin function */
const struct builtincmd *
find_builtin(const char *name)
{
  /* Check if a name was actually given */
  if (name) {
    /* Fill in the key value to perform search on */
    struct builtincmd key;
    key.name = name;

    /* Perform the binary search */
    return bsearch(&key, builtin, NUM_BLTIN, sizeof(struct builtincmd), bltin_compare);
  }
  return NULL;
}

static inline bool
in_builtin(const struct builtincmd *ptr)
{
  return (ptr && ptr >= &builtin[0] && ptr < &builtin[NUM_BLTIN]);
}

const char *
builtin_name(const struct builtincmd *bltin)
{
  if (!in_builtin(bltin)) return NULL;
  return bltin->name;
}

unsigned short
builtin_flags(const struct builtincmd *bltin)
{
  if (!in_builtin(bltin)) return 0;
  return bltin->flags;
}

bool
builtin_isspecial(const struct builtincmd *bltin)
{
  if (!in_builtin(bltin)) return false;
  return bltin->flags & 1;
}

bool
builtin_regular(const struct builtincmd *bltin)
{
  if (!in_builtin(bltin)) return false;
  return bltin->flags & 2;
}

bool
builtin_assign(const struct builtincmd *bltin)
{
  if (!in_builtin(bltin)) return false;
  return bltin->flags & 4;
}
