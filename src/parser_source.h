#ifndef PARSER_SOURC_H
#define PARSER_SOURC_H

#include <stdbool.h>

struct parser_source {
    const char *text;

    const char *filedir;
    const char *filename;
    const char *modulename;

    char *file_content;
};

struct parser_source_stack {
    struct parser_source **data;
    int cap;
    int len;
};

void parser_source_init(struct parser_source *source,
        const char *text, const char *filename, const char *modulename);
void parser_source_clear(struct parser_source *src);

bool parser_source_from_file(struct parser_source *src, const char *filepath);

void parser_source_stack_init(struct parser_source_stack *v);
bool parser_source_stack_is_empty(const struct parser_source_stack *v);
void parser_source_stack_push(struct parser_source_stack *v, struct parser_source *val);
void parser_source_stack_clear(struct parser_source_stack *v);

#endif /* _H */
