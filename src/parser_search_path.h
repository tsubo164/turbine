#ifndef PARSER_SEARCH_PATH_H
#define PARSER_SEARCH_PATH_H

struct builtin_module_list;

struct parser_search_path {
    const char *filedir;
    const struct builtin_module_list *builtin_modules;
};

void parser_search_path_init(struct parser_search_path *paths, const char *filedir);
void parser_search_path_free(struct parser_search_path *paths);

char *parser_search_path_find(const struct parser_search_path *paths, const char *filename);
void parser_search_path_add_builtin_modules(struct parser_search_path *paths,
        const struct builtin_module_list *modules);

#endif /* _H */
