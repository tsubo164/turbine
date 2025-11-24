#include "compile_context.h"
#include <stdlib.h>

void compile_context_init(struct compile_context *ctx)
{
    parser_token_pool_init(&ctx->token_pool);
    parser_node_pool_init(&ctx->node_pool);

    parser_source_stack_init(&ctx->sources);

    struct parser_search_dirs init = {0};
    ctx->search_dirs = init;
}

void compile_context_clear(struct compile_context *ctx)
{
    parser_token_pool_clear(&ctx->token_pool);
    parser_node_pool_clear(&ctx->node_pool);

    parser_source_stack_clear(&ctx->sources);

    parser_search_dirs_clear(&ctx->search_dirs);
}

struct parser_source *compile_context_read_file(struct compile_context *ctx,
        const char *filedir, const char *filename, const char *modulename)
{
    /* file path */
    const char *filepath = parser_search_dirs_find(&ctx->search_dirs, filename);
    if (!filepath)
        return NULL;

    struct parser_source *src;

    src = calloc(1, sizeof(*src));
    /* TODO use parser_source_init() */
    src->filepath = filepath;
    src->filename = filename;
    src->filedir = filedir;
    src->modulename = modulename;

    parser_source_stack_push(&ctx->sources, src);

    bool found = parser_source_from_file(src, filepath);
    if (!found)
        return NULL;

    return src;
}
