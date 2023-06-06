/*
 * This source provides some functions that enable the use of the queue.h
 * macros without requiring the duplication of code.
 */

#if INLINE_QUEUE_FUNCS
#error "Only compile 'queue.c' if not using inline queue functions"
#else
#include <obstack.h>
#include "parser.h"
#include "stailq.h"
#include "dtailq.h"
#endif
