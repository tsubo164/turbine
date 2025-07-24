#ifndef RUNTIME_STRUCT_H
#define RUNTIME_STRUCT_H

#include "runtime_gc.h"
#include "runtime_value.h"

struct runtime_struct {
    struct runtime_object obj;
    struct runtime_valuevec fields;
    int id;
};

struct runtime_struct *runtime_struct_new(struct runtime_gc *gc, int id, value_int_t len);
void runtime_struct_free(struct runtime_gc *gc, struct runtime_struct *s);

value_int_t runtime_struct_field_count(const struct runtime_struct *s);
struct runtime_value runtime_struct_get(const struct runtime_struct *s, value_int_t field_idx);
void runtime_struct_set(struct runtime_struct *s, value_int_t field_idx, struct runtime_value val);

#endif /* _H */
