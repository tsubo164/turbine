#ifndef COMPILE_CONTEXT_H
#define COMPILE_CONTEXT_H

#include "parser_ast.h"
#include "parser_token.h"
#include "parser_import.h"

struct compile_context {
    struct parser_token_pool token_pool;
    struct parser_node_pool node_pool;

    struct parser_importvec imports;
};

void compile_context_init(struct compile_context *ctx);
void compile_context_clear(struct compile_context *ctx);

#endif /* _H */
