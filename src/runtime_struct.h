#ifndef RUNTIME_STRUCT_H
#define RUNTIME_STRUCT_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct runtime_struct {
    struct runtime_object obj;
    struct runtime_valuevec fields;
};

struct runtime_struct *runtime_struct_new(int64_t len);
void runtime_struct_free(struct runtime_struct *s);

int64_t runtime_struct_field_count(const struct runtime_struct *s);
struct runtime_value runtime_struct_get(const struct runtime_struct *s, int64_t field_idx);
void runtime_struct_set(struct runtime_struct *s, int64_t field_idx, struct runtime_value val);

#endif // _H
