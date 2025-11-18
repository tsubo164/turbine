#ifndef PARSER_IMPORT_H
#define PARSER_IMPORT_H

#include <stdbool.h>

struct parser_import {
    char *text;
    char *filepath;
    const char *modulename;
};

struct parser_importvec {
    struct parser_import *data;
    int cap;
    int len;
};

bool parser_import_file(struct parser_import *i, const char *filepath);

void parser_importvec_init(struct parser_importvec *v);
bool parser_importvec_is_empty(const struct parser_importvec *v);
void parser_importvec_resize(struct parser_importvec *v, int new_len);
void parser_importvec_push(struct parser_importvec *v, const struct parser_import *val);
void parser_importvec_clear(struct parser_importvec *v);

#endif /* _H */
