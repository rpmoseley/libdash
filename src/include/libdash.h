/*
 * This header provides the public interface to the libdash library.
 */

#pragma once

#include <stdbool.h>

/* Provide opaque types to library structures and unions */
union parse_node;
struct parse_context;

/* Provide the interface to the context used by the library */
bool parse_new(struct parse_context **);
void parse_free(struct parse_context **);
const char *parse_internal_errstr(struct parse_context *);
bool parse_push_string(struct parse_context *, const char *);
bool parse_push_file(struct parse_context *, const char *);
union parse_node *parse_next_command(struct parse_context *);
bool parse_node_iseof(union parse_node *);
