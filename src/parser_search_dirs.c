#include "parser_search_dirs.h"
#include "builtin_module.h"
#include "os.h"

#include <stdlib.h>

void parser_search_dirs_init(struct parser_search_dirs *dirs, const char *filedir)
{
    dirs->filedir = filedir;

    struct data_strbuf init = DATA_STRBUF_INIT;
    dirs->pathbuf = init;
}

void parser_search_dirs_clear(struct parser_search_dirs *dirs)
{
    data_strbuf_free(&dirs->pathbuf);
}

const char *parser_search_dirs_find(struct parser_search_dirs *dirs, const char *filename)
{
    data_strbuf_copy(&dirs->pathbuf, dirs->filedir);
    data_strbuf_push(&dirs->pathbuf, '/');
    data_strbuf_cat(&dirs->pathbuf, filename);

    const char *path = data_strbuf_get(&dirs->pathbuf);

    if (os_path_exists(path))
        return path;

    return NULL;
}

void parser_search_dirs_add_builtin_modules(struct parser_search_dirs *dirs,
        const struct builtin_module_list *modules)
{
    dirs->builtin_modules = modules;
}
