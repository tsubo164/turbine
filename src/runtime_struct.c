#include "runtime_struct.h"
#include <stdlib.h>

struct runtime_struct *runtime_struct_new(value_int_t len)
{
    struct runtime_struct *s;

    s = calloc(1, sizeof(*s));
    s->obj.kind = OBJ_STRUCT;

    runtime_valuevec_init(&s->fields);
    runtime_valuevec_resize(&s->fields, len);

    return s;
}

void runtime_struct_free(struct runtime_struct *s)
{
    if (!s)
        return;

    runtime_valuevec_free(&s->fields);
    free(s);
}

value_int_t runtime_struct_field_count(const struct runtime_struct *s)
{
    return s->fields.len;
}

struct runtime_value runtime_struct_get(const struct runtime_struct *s, value_int_t field_idx)
{
    if (field_idx < 0 || field_idx >= s->fields.len) {
        // TODO error
    }
    return s->fields.data[field_idx];
}

void runtime_struct_set(struct runtime_struct *s, value_int_t field_idx, struct runtime_value val)
{
    if (field_idx < 0 || field_idx >= s->fields.len) {
        // TODO error
    }
    s->fields.data[field_idx] = val;
}
