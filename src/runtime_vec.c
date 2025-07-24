#include "runtime_vec.h"
#include <stdlib.h>

struct runtime_vec *runtime_vec_new(struct runtime_gc *gc, int val_type, value_int_t len)
{
    struct runtime_vec *v;

    v = runtime_alloc_object(gc, OBJ_VEC, sizeof(*v));
    v->val_type = val_type;
    runtime_valuevec_init(&v->values);
    runtime_valuevec_resize(gc, &v->values, len);

    runtime_gc_push_object(gc, (struct runtime_object*) v);
    return v;
}

void runtime_vec_free(struct runtime_gc *gc, struct runtime_vec *v)
{
    if (!v)
        return;
    runtime_valuevec_free(gc, &v->values);
    runtime_gc_free(gc, v);
}

/* no index range check */
struct runtime_value runtime_vec_get(const struct runtime_vec *v, value_int_t idx)
{
    return v->values.data[idx];
}

void runtime_vec_set(struct runtime_vec *v, value_int_t idx, struct runtime_value val)
{
    v->values.data[idx] = val;
}

value_int_t runtime_vec_len(const struct runtime_vec *v)
{
    return v->values.len;
}

bool runtime_vec_is_valid_index(const struct runtime_vec *v, value_int_t idx)
{
    return idx >= 0 && idx < runtime_vec_len(v);
}

/* gc managed */
void runtime_vec_resize(struct runtime_gc *gc, struct runtime_vec *v, value_int_t new_len)
{
    runtime_valuevec_resize(gc, &v->values, new_len);
}

void runtime_vec_push(struct runtime_gc *gc, struct runtime_vec *v, struct runtime_value val)
{
    runtime_valuevec_push(gc, &v->values, val);
}

void runtime_vec_clear(struct runtime_gc *gc, struct runtime_vec *v)
{
    runtime_valuevec_resize(gc, &v->values, 0);
}
