#ifndef COMPILE_CONTEXT_H
#define COMPILE_CONTEXT_H

#include "parser_ast.h"
#include "parser_type.h"
#include "parser_token.h"
#include "parser_source.h"
#include "parser_search_dirs.h"
#include "data_intern.h"
#include "data_strbuf.h"

struct compile_context {
    /* memory */
    struct parser_token_pool token_pool;
    struct parser_node_pool node_pool;
    struct parser_type_pool type_pool;
    struct data_intern_table intern_table;

    /* sources */
    struct parser_source main_src;
    struct parser_sourcevec sources;

    /* search dirs */
    struct parser_search_dirs search_dirs;
    struct data_strbuf pathbuf;

    /* import stack */
    const struct parser_source **importstack;
    int importsp;
};

void compile_context_init(struct compile_context *ctx);
void compile_context_clear(struct compile_context *ctx);

const char *compile_context_find_dir(struct compile_context *ctx, const char *filename);

struct parser_source *compile_context_read_file(struct compile_context *ctx,
        const char *filedir, const char *filename, const char *modulename);

void compile_context_push_source(struct compile_context *ctx, const struct parser_source *src);
void compile_context_pop_source(struct compile_context *ctx);
bool compile_context_has_cyclic_import(const struct compile_context *ctx, const struct parser_source *src);
int compile_context_get_import_stack_count(const struct compile_context *ctx);
const struct parser_source *compile_context_get_stacked_source(const struct compile_context *ctx, int index);
void compile_context_set_main_source(struct compile_context *ctx, const struct parser_source *src);
const struct parser_source *compile_context_get_main_source(const struct compile_context *ctx);

#endif /* _H */
