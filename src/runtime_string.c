#include "runtime_string.h"

#include <string.h>
#include <stdlib.h>

struct runtime_string *new_string(char *new_data, int new_len)
{
    struct runtime_string *str;

    str = calloc(1, sizeof(*str));
    str->obj.kind = OBJ_STRING;
    str->data = new_data;
    str->len = new_len;

    return str;
}

struct runtime_string *runtime_string_new(const char *s)
{
    char *data = strdup(s);
    char len = strlen(s);

    return new_string(data, len);
}

void runtime_string_free(struct runtime_string *str)
{
    if (!str)
        return;
    free(str->data);
    free(str);
}

int runtime_string_compare_cstr(const struct runtime_string *str, const char *cstr)
{
    if (!str)
        return -1;
    return strcmp(str->data, cstr);
}

int runtime_string_compare(const struct runtime_string *a, const struct runtime_string *b)
{
    return strcmp(a->data, b->data);
}

struct runtime_string *runtime_string_concat(const struct runtime_string *a,
        const struct runtime_string *b)
{
    int new_len = a->len + b->len + 1;
    char *new_data;

    new_data = calloc(new_len, sizeof(*new_data));
    memcpy(new_data, a->data, a->len);
    memcpy(new_data + a->len, b->data, b->len);

    return new_string(new_data, new_len);
}

const char *runtime_string_get_cstr(const struct runtime_string *s)
{
    if (!s)
        return NULL;
    return s->data;
}
