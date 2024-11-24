#include "parser_search_path.h"

void parser_search_path_init(struct parser_search_path *paths, const char *filedir)
{
    paths->filedir = filedir;
}

void parser_search_path_free(struct parser_search_path *paths)
{
}
