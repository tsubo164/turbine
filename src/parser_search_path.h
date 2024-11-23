#ifndef PARSER_SEARCH_PATH_H
#define PARSER_SEARCH_PATH_H

struct parser_search_path {
    const char *filename;
    char *current_directory;
    char *filepath;
    char *filedir;
};

void parser_search_path_free(struct parser_search_path *paths);
void parser_search_path_init(struct parser_search_path *paths, const char *filename);

#endif /* _H */
