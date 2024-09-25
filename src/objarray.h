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

#endif // _H
