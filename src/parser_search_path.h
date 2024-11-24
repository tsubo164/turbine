#ifndef PARSER_SEARCH_PATH_H
#define PARSER_SEARCH_PATH_H

struct parser_search_path {
    const char *filedir;
};

void parser_search_path_init(struct parser_search_path *paths, const char *filedir);
void parser_search_path_free(struct parser_search_path *paths);

char *parser_search_path_find(const struct parser_search_path *paths, const char *filename);

#endif /* _H */
