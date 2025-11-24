#ifndef COMPILE_CONTEXT_H
#define COMPILE_CONTEXT_H

#include "parser_ast.h"
#include "parser_token.h"
#include "parser_source.h"
#include "parser_search_dirs.h"
#include "data_strbuf.h"

struct compile_context {
    struct parser_token_pool token_pool;
    struct parser_node_pool node_pool;

    struct parser_source_stack sources;

    struct parser_search_dirs search_dirs;
    struct data_strbuf pathbuf;
};

void compile_context_init(struct compile_context *ctx);
void compile_context_clear(struct compile_context *ctx);

const char *compile_context_find_dir(struct compile_context *ctx, const char *filename);

struct parser_source *compile_context_read_file(struct compile_context *ctx,
        const char *filedir, const char *filename, const char *modulename);

#endif /* _H */
