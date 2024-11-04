#include "runtime_struct.h"
#include "mem.h"
#include <stdio.h>

struct runtime_struct *runtime_struct_new(int64_t len)
{
    struct runtime_struct *s = CALLOC(struct runtime_struct);
    s->obj.kind = OBJ_STRUCT;

    ValueVecInit(&s->values);
    ValueVecResize(&s->values, len);
    runtime_valuevec_zeroclear(&s->values);

    return s;
}

void runtime_struct_free(struct runtime_struct *s)
{
}

struct Value runtime_struct_get(const struct runtime_struct *s, int64_t field_idx)
{
    if (field_idx < 0 || field_idx >= s->values.len) {
        // todo error
    }
    return s->values.data[field_idx];
}

void runtime_struct_set(struct runtime_struct *s, int64_t field_idx, struct Value val)
{
    if (field_idx < 0 || field_idx >= s->values.len) {
        // todo error
    }
    s->values.data[field_idx] = val;
}

#ifdef TEST
Runtime_Struct *runtime_struct_new(int64_t len)
{
    Runtime_Struct *s = CALLOC(Runtime_Struct);

    value_vec_init(&s->values);
    value_vec_resize(&s->values, len);

    return s;
}

Value runtime_struct_get(const Runtime_Struct *s, int64_t field_idx)
{
    if (field_idx < 0 || field_idx >= s->values.len) {
        // todo error
    }
    return s->values.data[field_idx];
}

void runtime_struct_set(Runtime_Struct *s, int64_t field_idx, Value val)
{
    if (field_idx < 0 || field_idx >= s->values.len) {
        // todo error
    }
    s->values.data[field_idx] = val;
}

void runtime_struct_free(Runtime_Struct *s)
{
}
#endif
