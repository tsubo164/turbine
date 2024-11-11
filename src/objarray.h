#ifndef OBJARRAY_H
#define OBJARRAY_H

#include "gc.h"
#include <stdint.h>

struct ObjArray {
    struct runtime_object obj;
    // TODO remove this
    struct runtime_value *values;
    int64_t *data;
    int64_t len;
    int64_t cap;
};

struct ObjArray *NewArray(struct GC *gc, int64_t len);
struct runtime_value ArrayIndex(struct ObjArray *array, int64_t index);

struct GCArray {
    struct runtime_object obj;
    struct runtime_valuevec values;
};

struct GCArray *ArrayNew(struct GC *gc, int64_t len);
struct runtime_value ArrayGet(const struct GCArray *a, int64_t idx);
void ArraySet(struct GCArray *a, int64_t idx, struct runtime_value val);
void ArrayFree(struct GC *gc, struct GCArray *a);

#endif // _H
