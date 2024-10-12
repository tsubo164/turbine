#include "gc.h"
#include "error.h"
#include "mem.h"
// TODO move this
#include "objarray.h"

#include <string.h>
#include <stdio.h>

static void print_obj(const Obj *obj)
{
    switch (obj->kind) {
    case OBJ_NIL:
        printf("[NilObj] => nil\n");
        break;

    case OBJ_STRING:
        printf("[StringObj] => %s\n", ((StringObj *) obj)->data);
        break;

    case OBJ_ARRAY_:
        {
            const struct ObjArray *array = (struct ObjArray *) obj;
            printf("[Array] => len: %lld, cap: %lld\n", array->len, array->cap);
        }
        break;

    case OBJ_ARRAY:
        {
            const struct GCArray *array = (struct GCArray *) obj;
            printf("[Array] => len: %d, cap: %d\n",
                    array->values.len, array->values.cap);
        }
        break;

    default:
        UNREACHABLE;
        break;
    }
}

StringObj *NewString(GC *gc, const char *s)
{
    StringObj *str = CALLOC(StringObj);
    str->data = strdup(s);

    str->obj.next = gc->root;
    gc->root = (Obj*)str;

    return str;
}

struct StringObj *GCStringNew(const char *s)
{
    struct StringObj *str = CALLOC(StringObj);
    str->data = strdup(s);

    return str;
}

void GCStringFree(struct StringObj *str)
{
    if (!str)
        return;
    free(str->data);
    free(str);
}

void PrintObjects(const GC *gc)
{
    for (Obj *obj = gc->root; obj; obj = obj->next) {
        print_obj(obj);
    }
}
