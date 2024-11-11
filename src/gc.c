#include "gc.h"
#include "error.h"
#include "mem.h"
// TODO move this
#include "runtime_array.h"
#include "runtime_string.h"
#include "runtime_struct.h"

#include <string.h>
#include <stdio.h>

static void print_obj(const struct runtime_object *obj)
{
    switch (obj->kind) {
    case OBJ_NIL:
        printf("[NilObj] => nil\n");
        break;

    case OBJ_STRING:
        {
            const struct runtime_string *s = (struct runtime_string *) obj;
            printf("[String] => %s\n", runtime_string_get_cstr(s));
        }
        break;

        /*
    case OBJ_ARRAY_:
        {
            const struct ObjArray *array = (struct ObjArray *) obj;
            printf("[Array] => len: %lld, cap: %lld\n", array->len, array->cap);
        }
        break;
        */

    case OBJ_ARRAY:
        {
            const struct runtime_array *array = (struct runtime_array *) obj;
            printf("[Array] => len: %d, cap: %d\n",
                    array->values.len, array->values.cap);
        }
        break;

    case OBJ_STRUCT:
        {
            const struct runtime_struct *s = (struct runtime_struct *) obj;
            printf("[Struct] => len: %d, cap: %d\n", s->values.len, s->values.cap);
        }
        break;

    default:
        UNREACHABLE;
        break;
    }
}

/*
StringObj *NewString(GC *gc, const char *s)
{
    StringObj *str = CALLOC(StringObj);
    str->obj.kind = OBJ_STRING;
    str->data = strdup(s);

    str->obj.next = gc->root;
    gc->root = (runtime_object*)str;

    return str;
}

struct StringObj *new_string(char *new_data, int new_len)
{
    struct StringObj *str = CALLOC(StringObj);
    str->obj.kind = OBJ_STRING;
    str->data = new_data;
    str->len = new_len;

    return str;
}

struct StringObj *GCStringNew(const char *s)
{
    char *data = strdup(s);
    char len = strlen(s);

    return new_string(data, len);
}

void GCStringFree(struct StringObj *str)
{
    if (!str)
        return;
    free(str->data);
    free(str);
}

int runtime_string_compare_cstr(const struct StringObj *str, const char *cstr)
{
    if (!str)
        return -1;
    return strcmp(str->data, cstr);
}

int runtime_string_compare(const struct StringObj *a, const struct StringObj *b)
{
    return strcmp(a->data, b->data);
}

struct StringObj *runtime_string_concat(const struct StringObj *a, const struct StringObj *b)
{
    int new_len = a->len + b->len + 1;
    char *new_data = NALLOC(new_len, char);

    memcpy(new_data, a->data, a->len);
    memcpy(new_data + a->len, b->data, b->len);

    return new_string(new_data, new_len);
}

const char *runtime_string_get_cstr(const struct StringObj *s)
{
    if (!s)
        return NULL;
    return s->data;
}
*/

void runtime_append_gc_object(struct GC *gc, struct runtime_object *obj)
{
    obj->next = gc->root;
    gc->root = obj;
}

void PrintObjects(const GC *gc)
{
    for (struct runtime_object *obj = gc->root; obj; obj = obj->next) {
        print_obj(obj);
    }
}
