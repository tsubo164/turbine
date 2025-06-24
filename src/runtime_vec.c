#include "runtime_vec.h"
#include <stdlib.h>

struct runtime_vec *runtime_vec_new(int val_type, value_int_t len)
{
    struct runtime_vec *v;

    v = runtime_alloc_object(OBJ_VEC, sizeof(*v));
    v->val_type = val_type;
    runtime_valuevec_init(&v->values);
    runtime_valuevec_resize(&v->values, len);

    return v;
}

void runtime_vec_free(struct runtime_vec *v)
{
    if (!v)
        return;

    runtime_valuevec_free(&v->values);
    free(v);
}

/* No index range check */
struct runtime_value runtime_vec_get(const struct runtime_vec *v, value_int_t idx)
{
    return v->values.data[idx];
}

void runtime_vec_set(struct runtime_vec *v, value_int_t idx, struct runtime_value val)
{
    v->values.data[idx] = val;
}

void runtime_vec_push(struct runtime_vec *v, struct runtime_value val)
{
    runtime_valuevec_push(&v->values, val);
}

void runtime_vec_clear(struct runtime_vec *v)
{
    runtime_valuevec_resize(&v->values, 0);
}

value_int_t runtime_vec_len(const struct runtime_vec *v)
{
    return v->values.len;
}

void runtime_vec_resize(struct runtime_vec *v, value_int_t new_len)
{
    runtime_valuevec_resize(&v->values, new_len);
}

bool runtime_vec_is_valid_index(const struct runtime_vec *v, value_int_t idx)
{
    return idx >= 0 && idx < runtime_vec_len(v);
}
