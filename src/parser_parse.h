#ifndef PARSER_PARSE_H
#define PARSER_PARSE_H

#include "parser_ast.h"
#include "parser_search_path.h"
#include "compile_context.h"

struct parser_module;
struct parser_scope;
struct parser_token;

struct parser_source {
    const char *text;
    const char *filename;
    const char *modulename;
};

struct parser_module *parser_parse(const struct parser_token *tok,
        struct parser_scope *scope,
        const struct parser_source *source,
        const struct parser_search_path *paths,
        struct compile_context *ctx);

void parser_source_init(struct parser_source *source,
        const char *text, const char *filename, const char *modulename);

#endif /* _H */
