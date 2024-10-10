#include "objarray.h"
#include "mem.h"
#include <stdio.h>

struct ObjArray *NewArray(struct GC *gc, int64_t len)
{
    struct ObjArray *array = CALLOC(struct ObjArray);

    int64_t cap = 8;
    while (cap < len)
        cap *= 2;
    array->data = NALLOC(cap, *array->data);
    array->cap = cap;
    array->len = len;

    array->obj.kind = OBJ_ARRAY_;
    array->obj.next = gc->root;
    gc->root = (Obj*)array;

    return array;
}

struct Value ArrayIndex(struct ObjArray *array, int64_t index)
{
    struct Value val = {0};

    return val;
}

struct GCArray *ArrayNew(struct GC *gc, int64_t len)
{
    struct GCArray *array = CALLOC(struct GCArray);

    ValueVecInit(&array->values);
    ValueVecResize(&array->values, len);

    array->obj.kind = OBJ_ARRAY;
    array->obj.next = gc->root;
    gc->root = (struct Obj*)array;

    return array;
}

struct Value ArrayGet(const struct GCArray *a, int64_t idx)
{
    if (idx < 0 || idx >= a->values.len) {
        // TODO error
    }
    return a->values.data[idx];
}

void ArraySet(struct GCArray *a, int64_t idx, struct Value val)
{
    if (idx < 0 || idx >= a->values.len) {
        // TODO error
    }
    a->values.data[idx] = val;
}

void ArrayFree(struct GC *gc, struct GCArray *a)
{
}
