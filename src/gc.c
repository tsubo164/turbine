#include "gc.h"
#include "error.h"
#include "mem.h"
// TODO move this
#include "objarray.h"

#include <string.h>
#include <stdio.h>

void ValueVecReisze(struct ValueVec *v, int new_len)
{
    if (new_len >= v->cap) {
        v->cap = v->cap < 8 ? 8 : v->cap;
        while (v->cap < new_len)
            v->cap *= 2;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->len = new_len;
}

void ValueVecPush(struct ValueVec *v, Value val)
{
    if (v->len >= v->cap) {
        v->cap = v->cap < 8 ? 8 : v->cap * 2;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

static void print_obj(const Obj *obj)
{
    switch (obj->kind) {
    case OBJ_NIL:
        printf("[NilObj] => nil\n");
        break;

    case OBJ_STRING:
        printf("[StringObj] => %s\n", ((StringObj *) obj)->data);
        break;

    case OBJ_ARRAY:
        {
            const struct ObjArray *array = (struct ObjArray *) obj;
            printf("[Array] => len: %lld, cap: %lld\n", array->len, array->cap);
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

void PrintObjects(const GC *gc)
{
    for (Obj *obj = gc->root; obj; obj = obj->next) {
        print_obj(obj);
    }
}
