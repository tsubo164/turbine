#ifndef PARSER_SEARCH_DIRS_H
#define PARSER_SEARCH_DIRS_H

#include "data_strbuf.h"

struct builtin_module_list;

struct parser_search_dirs {
    const char *filedir;
    const struct builtin_module_list *builtin_modules;

    struct data_strbuf pathbuf;
};

void parser_search_dirs_init(struct parser_search_dirs *dirs, const char *filedir);
void parser_search_dirs_clear(struct parser_search_dirs *dirs);

const char *parser_search_dirs_find(struct parser_search_dirs *dirs, const char *filename);
void parser_search_dirs_add_builtin_modules(struct parser_search_dirs *dirs,
        const struct builtin_module_list *modules);

#endif /* _H */
