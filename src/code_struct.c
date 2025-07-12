#include "code_struct.h"
#include <stdlib.h>
#include <assert.h>

#define MIN_CAP 8

int code_push_struct(struct code_structvec *v, const char *fullname, int field_count)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }

    int new_id = v->len++;
    struct code_struct init = {0};
    struct code_struct *strct = &v->data[new_id];

    *strct = init;
    strct->id = new_id;
    strct->field_count = field_count;
    strct->fullname = fullname;

    return new_id;
}

struct code_struct *code_lookup_struct(struct code_structvec *v, int id)
{
    if (id < 0 || id >= v->len)
        return NULL;

    return &v->data[id];
}

const struct code_struct *code_lookup_const_struct(const struct code_structvec *v, int id)
{
    if (id < 0 || id >= v->len)
        return NULL;

    return &v->data[id];
}

void code_struct_push_value_type(struct code_struct *s, int val_type)
{
    data_strbuf_push(&s->val_types, val_type);
}

int code_struct_get_value_type(const struct code_struct *s, int field_id)
{
    assert(field_id >= 0 && field_id < data_strbuf_len(&s->val_types));
    /* TODO need data_strbuf_get()? */
    return s->val_types.data[field_id];
}

int code_struct_get_field_count(const struct code_struct *s)
{
    return data_strbuf_len(&s->val_types);
}

void code_structvec_free(struct code_structvec *v)
{
    for (int i = 0; i < v->len; i++) {
        struct code_struct *s = code_lookup_struct(v, i);
        data_strbuf_free(&s->val_types);
    }

    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}
