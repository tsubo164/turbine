#include "parser_import.h"
#include "read_file.h"
#include <stdlib.h>

bool parser_import_file(struct parser_import *imp, char *filepath)
{
    char *text = read_file(filepath);
    if (!text)
        return false;

    imp->text = text;
    imp->filepath = filepath;

    return true;
}

static void clear_import(struct parser_import *imp)
{
    free(imp->text);
    free(imp->filepath);
}

#define MIN_CAP 8

void parser_importvec_init(struct parser_importvec *v)
{
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

bool parser_importvec_is_empty(const struct parser_importvec *v)
{
    return v->len == 0;
}

void parser_importvec_resize(struct parser_importvec *v, int new_len)
{
    if (new_len <= v->cap) {
        v->len = new_len;
        return;
    }

    int new_cap = v->cap < (MIN_CAP) ? (MIN_CAP) : v->cap;
    while (new_cap < new_len)
        new_cap *= 2;

    v->data = realloc(v->data, sizeof(*v->data) * new_cap);
    v->cap = new_cap;
    v->len = new_len;
}

void parser_importvec_push(struct parser_importvec *v, const struct parser_import *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = *val;
}

void parser_importvec_clear(struct parser_importvec *v)
{
    for (int i = 0; i < v->len; i++) {
        clear_import(&v->data[i]);
    }

    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}
