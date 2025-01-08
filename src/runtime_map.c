#include "runtime_map.h"
#include <stdlib.h>

struct runtime_map *runtime_map_new(int64_t len)
{
    struct runtime_map *a;

    a = calloc(1, sizeof(*a));
    a->obj.kind = OBJ_MAP;

    runtime_valuevec_init(&a->values);
    runtime_valuevec_resize(&a->values, len);

    return a;
}

void runtime_map_free(struct runtime_map *a)
{
    if (!a)
        return;

    runtime_valuevec_free(&a->values);
    free(a);
}

struct runtime_value runtime_map_get(const struct runtime_map *a, const char *key)
{
    return (struct runtime_value) {0};
    /*
    return a->values.data[key];
    */
}

void runtime_map_set(struct runtime_map *a, const char *key, struct runtime_value val)
{
    /*
    a->values.data[key] = val;
    */
}

int64_t runtime_map_len(const struct runtime_map *a)
{
    return a->values.len;
}

void runtime_map_resize(struct runtime_map *a, int64_t new_len)
{
    runtime_valuevec_resize(&a->values, new_len);
}
