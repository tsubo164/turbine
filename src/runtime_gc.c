#include "runtime_gc.h"
#include "runtime_array.h"
#include "runtime_string.h"
#include "runtime_struct.h"

#include <assert.h>
#include <stdio.h>

void runtime_push_gc_object(struct runtime_gc *gc, struct runtime_object *obj)
{
    obj->next = gc->root;
    gc->root = obj;
}

static void print_obj(const struct runtime_object *obj)
{
    enum runtime_object_kind kind = obj->kind;

    switch (kind) {

    case OBJ_NIL:
        printf("[nil] => nil\n");
        break;

    case OBJ_STRING:
        {
            const struct runtime_string *s = (struct runtime_string *) obj;
            printf("[string] => %s\n", runtime_string_get_cstr(s));
        }
        break;

    case OBJ_ARRAY:
        {
            const struct runtime_array *a = (struct runtime_array *) obj;
            printf("[array] => len: %d, cap: %d\n",
                    a->values.len, a->values.cap);
        }
        break;

    case OBJ_STRUCT:
        {
            const struct runtime_struct *s = (struct runtime_struct *) obj;
            printf("[struct] => len: %d, cap: %d\n", s->values.len, s->values.cap);
        }
        break;

    default:
        assert(!"unreachable");
        break;
    }
}

void runtime_print_gc_objects(const struct runtime_gc *gc)
{
    for (struct runtime_object *obj = gc->root; obj; obj = obj->next) {
        print_obj(obj);
    }
}
