#include "objarray.h"
#include "mem.h"

struct ObjArray *NewArray(struct GC *gc, int64_t len)
{
    struct ObjArray *array = CALLOC(struct ObjArray);

    int64_t cap = 8;
    while (cap < len)
        cap *= 2;
    array->data = NALLOC(cap, *array->data);
    array->cap = cap;
    array->len = len;

    array->obj.kind = OBJ_ARRAY;
    array->obj.next = gc->root;
    gc->root = (Obj*)array;

    return array;
}

struct Value ArrayIndex(struct ObjArray *array, int64_t index)
{
    struct Value val = {0};

    return val;
}
