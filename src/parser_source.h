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

struct parser_sourcevec {
    struct parser_source **data;
    int cap;
    int len;
};

void parser_source_init(struct parser_source *source,
        const char *text, const char *filename, const char *modulename);
void parser_source_clear(struct parser_source *src);

bool parser_source_from_file(struct parser_source *src, const char *filepath);

/* source vec */
void parser_sourcevec_init(struct parser_sourcevec *v);
bool parser_sourcevec_is_empty(const struct parser_sourcevec *v);
void parser_sourcevec_push(struct parser_sourcevec *v, struct parser_source *val);
void parser_sourcevec_clear(struct parser_sourcevec *v);

#endif /* _H */
