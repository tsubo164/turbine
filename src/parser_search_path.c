#include "parser_search_path.h"
#include "builtin_module.h"
#include "os.h"

#include <stdlib.h>

void parser_search_path_init(struct parser_search_path *paths, const char *filedir)
{
    paths->filedir = filedir;
}

void parser_search_path_free(struct parser_search_path *paths)
{
}

char *parser_search_path_find(const struct parser_search_path *paths, const char *filename)
{
    char *filepath = os_path_join(paths->filedir, filename);

    if (os_path_exists(filepath))
        return filepath;

    free(filepath);
    return NULL;
}

void parser_search_path_add_builtin_modules(struct parser_search_path *paths,
        const struct builtin_module_list *modules)
{
    paths->builtin_modules = modules;
}
