#ifndef OBJARRAY_H
#define OBJARRAY_H

#include "gc.h"
#include <stdint.h>

struct ObjArray {
    Obj obj;
    // TODO remove this
    struct Value *values;
    int64_t *data;
    int64_t len;
    int64_t cap;
};

struct ObjArray *NewArray(struct GC *gc, int64_t len);
struct Value ArrayIndex(struct ObjArray *array, int64_t index);

struct GCArray {
    struct Obj obj;
    struct ValueVec values;
};

struct GCArray *ArrayNew(struct GC *gc, int64_t len);
struct Value ArrayGet(const struct GCArray *a, int64_t idx);
void ArraySet(struct GCArray *a, int64_t idx, struct Value val);
void ArrayFree(struct GC *gc, struct GCArray *a);

#endif // _H
