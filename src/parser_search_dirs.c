#include "parser_search_dirs.h"
#include "builtin_module.h"
#include "os.h"

#include <stdlib.h>

void parser_search_dirs_init(struct parser_search_dirs *dirs, const char *filedir)
{
    dirs->filedir = filedir;
}

void parser_search_dirs_clear(struct parser_search_dirs *dirs)
{
}

char *parser_search_dirs_find(const struct parser_search_dirs *dirs, const char *filename)
{
    char *filepath = os_path_join(dirs->filedir, filename);

    if (os_path_exists(filepath))
        return filepath;

    free(filepath);
    return NULL;
}

void parser_search_dirs_add_builtin_modules(struct parser_search_dirs *dirs,
        const struct builtin_module_list *modules)
{
    dirs->builtin_modules = modules;
}
