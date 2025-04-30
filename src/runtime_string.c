#include "runtime_string.h"
#include "data_cstr.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    char *data = data_strdup(s);
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

int runtime_string_len(const struct runtime_string *s)
{
    return s->len;
}

struct runtime_string *runtime_string_concat(const struct runtime_string *a,
        const struct runtime_string *b)
{
    int new_len = a->len + b->len;
    char *new_data;

    new_data = malloc((new_len + 1) * sizeof(*new_data));
    strcpy(new_data, a->data);
    strcpy(new_data + a->len, b->data);

    return new_string(new_data, new_len);
}

const char *runtime_string_get_cstr(const struct runtime_string *s)
{
    if (!s)
        return NULL;
    return s->data;
}
