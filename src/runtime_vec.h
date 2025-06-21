#ifndef RUNTIME_VEC_H
#define RUNTIME_VEC_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct runtime_vec {
    struct runtime_object obj;
    struct runtime_valuevec values;
    int val_type;
};

struct runtime_vec *runtime_vec_new(int val_type, value_int_t len);
void runtime_vec_free(struct runtime_vec *v);

/* No index range check */
struct runtime_value runtime_vec_get(const struct runtime_vec *v, value_int_t idx);
void runtime_vec_set(struct runtime_vec *v, value_int_t idx, struct runtime_value val);
void runtime_vec_push(struct runtime_vec *v, struct runtime_value val);
void runtime_vec_clear(struct runtime_vec *v);

value_int_t runtime_vec_len(const struct runtime_vec *v);
void runtime_vec_resize(struct runtime_vec *v, value_int_t new_len);
bool runtime_vec_is_valid_index(const struct runtime_vec *v, value_int_t idx);

#endif /* _H */
