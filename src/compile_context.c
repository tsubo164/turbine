#include "compile_context.h"

void compile_context_init(struct compile_context *ctx)
{
    parser_token_pool_init(&ctx->token_pool);
    parser_node_pool_init(&ctx->node_pool);

    parser_importvec_init(&ctx->imports);
}

void compile_context_clear(struct compile_context *ctx)
{
    parser_token_pool_clear(&ctx->token_pool);
    parser_node_pool_clear(&ctx->node_pool);

    parser_importvec_clear(&ctx->imports);
}
