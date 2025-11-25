#include "compile_context.h"
#include "os.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void compile_context_init(struct compile_context *ctx)
{
    parser_token_pool_init(&ctx->token_pool);
    parser_node_pool_init(&ctx->node_pool);

    parser_sourcevec_init(&ctx->sources);

    /* search dirs */
    struct parser_search_dirs dirsinit = {0};
    ctx->search_dirs = dirsinit;
    struct data_strbuf sbufinit = DATA_STRBUF_INIT;
    ctx->pathbuf = sbufinit;

    /* import stack */
    memset(ctx->importstack, 0, sizeof(ctx->importstack));
    ctx->importsp = -1;
}

void compile_context_clear(struct compile_context *ctx)
{
    parser_token_pool_clear(&ctx->token_pool);
    parser_node_pool_clear(&ctx->node_pool);

    parser_sourcevec_clear(&ctx->sources);

    /* search dirs */
    parser_search_dirs_clear(&ctx->search_dirs);
    data_strbuf_free(&ctx->pathbuf);
}

const char *compile_context_find_dir(struct compile_context *ctx, const char *filename)
{
    return parser_search_dirs_find(&ctx->search_dirs, filename);
}

struct parser_source *compile_context_read_file(struct compile_context *ctx,
        const char *filedir, const char *filename, const char *modulename)
{
    char dir_sep = os_dir_sep();

    data_strbuf_copy(&ctx->pathbuf, filedir);
    data_strbuf_push(&ctx->pathbuf, dir_sep);
    data_strbuf_cat(&ctx->pathbuf, filename);

    struct parser_source *src;

    src = calloc(1, sizeof(*src));
    /* TODO use parser_source_init() */
    src->filename = filename;
    src->filedir = filedir;
    src->modulename = modulename;

    parser_sourcevec_push(&ctx->sources, src);

    const char *filepath = data_strbuf_get(&ctx->pathbuf);
    bool ok = parser_source_from_file(src, filepath);
    if (!ok)
        return NULL;

    return src;
}

void compile_context_push_source(struct compile_context *ctx, const struct parser_source *src)
{
    int max_size = sizeof(ctx->importstack)/sizeof(ctx->importstack[0]);
    assert(ctx->importsp < max_size - 1);

    ctx->importstack[++ctx->importsp] = src;
}

void compile_context_pop_source(struct compile_context *ctx)
{
    assert(ctx->importsp >= 0);
    ctx->importstack[ctx->importsp--] = NULL;
}
