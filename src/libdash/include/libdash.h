/*
 * This header provides the public interface to the libdash library.
 */

#pragma once

#include <stdbool.h>

/* Provide opaque types to library structures and unions */
struct parse_context;
#ifdef USE_WALKINFO
struct parse_walkinfo;
#else
union parse_node;
#endif

/* Provide the interface to the context used by the library */
bool parse_new(struct parse_context **);
void parse_free(struct parse_context **);
const char *parse_internal_errstr(struct parse_context *);
bool parse_push_string(struct parse_context *, const char *);
bool parse_push_file(struct parse_context *, const char *);
#ifdef USE_WALKINFO
bool parse_iseof(struct parse_walkinfo *);
struct parse_walkinfo *parse_next_command(struct parse_context *);
bool parse_walk_next(struct parse_walkinfo *);
bool parse_walk_free(struct parse_walkinfo **);
int parse_walk_type(struct parse_walkinfo *);
#else
bool parse_iseof(union parse_node *);
union parse_node *parse_next_command(struct parse_context *);
#endif
