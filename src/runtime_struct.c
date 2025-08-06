#include "runtime_struct.h"
#include <stdlib.h>

struct runtime_struct *runtime_struct_new(struct runtime_gc *gc, int id, value_int_t len)
{
    struct runtime_struct *s;

    s = runtime_alloc_object(gc, OBJ_STRUCT, sizeof(*s));
    s->id = id;
    runtime_valuevec_init(&s->fields);
    runtime_valuevec_resize(gc, &s->fields, len);

    runtime_gc_push_object(gc, (struct runtime_object*) s);
    return s;
}

void runtime_struct_free(struct runtime_gc *gc, struct runtime_struct *s)
{
    if (!s)
        return;
    runtime_valuevec_free(gc, &s->fields);
    runtime_gc_free(gc, s);
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
