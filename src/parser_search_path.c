#include <stdlib.h>

#include "parser_search_path.h"
#include "os.h"

void parser_search_path_init(struct parser_search_path *paths, const char *filename)
{
    paths->filename = filename;
    paths->current_directory = get_current_directory();
    paths->filepath = os_path_join(paths->current_directory, paths->filename);
    paths->filedir = os_dirname(paths->filepath);
}

void parser_search_path_free(struct parser_search_path *paths)
{
    free(paths->current_directory);
    free(paths->filepath);
    free(paths->filedir);
}
