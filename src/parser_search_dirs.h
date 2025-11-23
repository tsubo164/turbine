#ifndef PARSER_SEARCH_DIRS_H
#define PARSER_SEARCH_DIRS_H

struct builtin_module_list;

struct parser_search_dirs {
    const char *filedir;
    const struct builtin_module_list *builtin_modules;
};

void parser_search_dirs_init(struct parser_search_dirs *dirs, const char *filedir);
void parser_search_dirs_clear(struct parser_search_dirs *dirs);

char *parser_search_dirs_find(const struct parser_search_dirs *dirs, const char *filename);
void parser_search_dirs_add_builtin_modules(struct parser_search_dirs *dirs,
        const struct builtin_module_list *modules);

#endif /* _H */
