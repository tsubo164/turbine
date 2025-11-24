#ifndef PARSER_PARSE_H
#define PARSER_PARSE_H

#include "parser_ast.h"
#include "parser_source.h"
#include "compile_context.h"

struct parser_module;
struct parser_scope;
struct parser_token;

struct parser_module *parser_parse(const struct parser_token *tok,
        struct parser_scope *scope,
        const struct parser_source *source,
        struct compile_context *ctx);

#endif /* _H */
