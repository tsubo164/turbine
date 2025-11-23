#include "compile_context.h"

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
