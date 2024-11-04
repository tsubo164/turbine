#ifndef RUNTIME_STRUCT_H
#define RUNTIME_STRUCT_H

#include "gc.h"
#include <stdint.h>

struct runtime_struct {
    struct Obj obj;
    struct runtime_valuevec values;
};

struct runtime_struct *runtime_struct_new(int64_t len);
void runtime_struct_free(struct runtime_struct *s);

struct runtime_value runtime_struct_get(const struct runtime_struct *s, int64_t field_idx);
void runtime_struct_set(struct runtime_struct *s, int64_t field_idx, struct runtime_value val);

#endif // _H
