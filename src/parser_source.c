#include "parser_source.h"
#include "read_file.h"
#include <stdlib.h>

bool parser_source_from_file(struct parser_source *src, char *filepath)
{
    char *content = read_file(filepath);
    if (!content)
        return false;
    src->file_content = content;

    src->text = src->file_content;
    src->filepath = filepath;

    return true;
}

void parser_source_clear(struct parser_source *src)
{
    free(src->file_content);
    free(src->filepath);
}

#define MIN_CAP 8

void parser_source_stack_init(struct parser_source_stack *v)
{
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

bool parser_source_stack_is_empty(const struct parser_source_stack *v)
{
    return v->len == 0;
}

void parser_source_stack_resize(struct parser_source_stack *v, int new_len)
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

void parser_source_stack_push(struct parser_source_stack *v, const struct parser_source *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = *val;
}

void parser_source_stack_clear(struct parser_source_stack *v)
{
    for (int i = 0; i < v->len; i++) {
        parser_source_clear(&v->data[i]);
    }

    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}
