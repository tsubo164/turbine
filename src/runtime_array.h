#ifndef RUNTIME_ARRAY_H
#define RUNTIME_ARRAY_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct runtime_array {
    struct runtime_object obj;
    struct runtime_valuevec values;
};

struct runtime_array *runtime_array_new(int64_t len);
void runtime_array_free(struct runtime_array *a);

/* No index range check */
struct runtime_value runtime_array_get(const struct runtime_array *a, int64_t idx);
void runtime_array_set(struct runtime_array *a, int64_t idx, struct runtime_value val);

int64_t runtime_array_len(const struct runtime_array *a);
void runtime_array_resize(struct runtime_array *a, int64_t new_len);
bool runtime_array_is_valid_index(const struct runtime_array *a, int64_t idx);

#endif // _H
