#include "runtime_array.h"
#include <stdlib.h>

struct runtime_array *runtime_array_new(int64_t len)
{
    struct runtime_array *a;

    a = calloc(1, sizeof(*a));
    a->obj.kind = OBJ_ARRAY;

    runtime_valuevec_init(&a->values);
    runtime_valuevec_resize(&a->values, len);

    return a;
}

void runtime_array_free(struct runtime_array *a)
{
    if (!a)
        return;

    runtime_valuevec_free(&a->values);
    free(a);
}

/* No index range check */
struct runtime_value runtime_array_get(const struct runtime_array *a, int64_t idx)
{
    return a->values.data[idx];
}

void runtime_array_set(struct runtime_array *a, int64_t idx, struct runtime_value val)
{
    a->values.data[idx] = val;
}

int64_t runtime_array_len(const struct runtime_array *a)
{
    return a->values.len;
}

void runtime_array_resize(struct runtime_array *a, int64_t new_len)
{
    runtime_valuevec_resize(&a->values, new_len);
}

bool runtime_array_is_valid_index(const struct runtime_array *a, int64_t idx)
{
    return idx >= 0 && idx < runtime_array_len(a);
}
