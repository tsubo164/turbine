#include "runtime_string.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct runtime_string *new_string(struct runtime_gc *gc, char *new_data, int new_len)
{
    struct runtime_string *s;

    s = runtime_alloc_object2(gc, OBJ_STRING, sizeof(*s));
    s->data = new_data;
    s->len = new_len;

    return s;
}

struct runtime_string *runtime_string_new(struct runtime_gc *gc, const char *cstr)
{
    size_t new_len = strlen(cstr);
    char *new_data;

    new_data = runtime_gc_alloc(gc, (new_len + 1) * sizeof(*new_data));
    if (new_data)
        strcpy(new_data, cstr);

    return new_string(gc, new_data, new_len);
}

void runtime_string_free(struct runtime_gc *gc, struct runtime_string *s)
{
    if (!s)
        return;
    runtime_gc_free(gc, s->data);
    runtime_gc_free(gc, s);
}

int runtime_string_compare_cstr(const struct runtime_string *s, const char *cstr)
{
    if (!s)
        return -1;
    return strcmp(s->data, cstr);
}

int runtime_string_compare(const struct runtime_string *a, const struct runtime_string *b)
{
    return strcmp(a->data, b->data);
}

int runtime_string_len(const struct runtime_string *s)
{
    return s->len;
}

struct runtime_string *runtime_string_concat(struct runtime_gc *gc,
        const struct runtime_string *a, const struct runtime_string *b)
{
    int new_len = a->len + b->len;
    char *new_data;

    new_data = runtime_gc_alloc(gc, (new_len + 1) * sizeof(*new_data));
    strcpy(new_data, a->data);
    strcpy(new_data + a->len, b->data);

    return new_string(gc, new_data, new_len);
}

const char *runtime_string_get_cstr(const struct runtime_string *s)
{
    if (!s)
        return NULL;
    return s->data;
}
