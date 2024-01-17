#include "gc.h"
#include "compiler.h"
#include "error.h"
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

void PrintObjs(const GC *gc)
{
    for (Obj *obj = gc->root; obj; obj = obj->next) {
        print_obj(obj);
    }
}
