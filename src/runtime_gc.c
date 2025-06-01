#include "runtime_gc.h"
#include "runtime_vec.h"
#include "runtime_map.h"
#include "runtime_set.h"
#include "runtime_stack.h"
#include "runtime_queue.h"
#include "runtime_string.h"
#include "runtime_struct.h"

#include <assert.h>
#include <stdio.h>

enum object_mark {
    OBJ_WHITE,
    OBJ_GRAY,
    OBJ_BLACK,
};

struct runtime_string *runtime_gc_string_new(struct runtime_gc *gc, const char *cstr)
{
    struct runtime_string *str = runtime_string_new(cstr);

    runtime_gc_push_object(gc, (struct runtime_object *) str);

    return str;
}

void runtime_gc_push_object(struct runtime_gc *gc, struct runtime_object *obj)
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

    case OBJ_VEC:
        {
            const struct runtime_vec *a = (struct runtime_vec *) obj;
            printf("[vec] => len: %d, cap: %d\n", a->values.len, a->values.cap);
        }
        break;

    case OBJ_MAP:
        {
            const struct runtime_map *m = (struct runtime_map *) obj;
            printf("[map] => len: %" PRIival ", cap: %" PRIival "\n", m->len, m->cap);
        }
        break;

    case OBJ_STRUCT:
        {
            const struct runtime_struct *s = (struct runtime_struct *) obj;
            printf("[struct] => len: %d, cap: %d\n", s->fields.len, s->fields.cap);
        }
        break;

    default:
        assert(!"unreachable");
        break;
    }
}

void runtime_gc_print_objects(const struct runtime_gc *gc)
{
    for (struct runtime_object *obj = gc->root; obj; obj = obj->next) {
        print_obj(obj);
    }
}

void runtime_gc_collect_objects(const struct runtime_gc *gc)
{
    /* clear marks */
    for (struct runtime_object *obj = gc->root; obj; obj = obj->next) {
        print_obj(obj);
        obj->mark = OBJ_WHITE;
    }

    /* track from roots */

    /* free white objects */
}

static void free_obj(struct runtime_object *obj)
{
    enum runtime_object_kind kind = obj->kind;

    switch (kind) {

    case OBJ_NIL:
        break;

    case OBJ_STRING:
        {
            struct runtime_string *s = (struct runtime_string *) obj;
            runtime_string_free(s);
        }
        break;

    case OBJ_VEC:
        {
            struct runtime_vec *v = (struct runtime_vec *) obj;
            runtime_vec_free(v);
        }
        break;

    case OBJ_MAP:
        {
            struct runtime_map *m = (struct runtime_map *) obj;
            runtime_map_free(m);
        }
        break;

    case OBJ_SET:
        {
            struct runtime_set *s = (struct runtime_set *) obj;
            runtime_set_free(s);
        }
        break;

    case OBJ_STACK:
        {
            struct runtime_stack *s = (struct runtime_stack *) obj;
            runtime_stack_free(s);
        }
        break;

    case OBJ_QUEUE:
        {
            struct runtime_queue *q = (struct runtime_queue *) obj;
            runtime_queue_free(q);
        }
        break;

    case OBJ_STRUCT:
        {
            struct runtime_struct *s = (struct runtime_struct *) obj;
            runtime_struct_free(s);
        }
        break;
    }
}

void runtime_gc_free(struct runtime_gc *gc)
{
    struct runtime_object *obj = gc->root;
    struct runtime_object *next = NULL;

    while (obj) {
        next = obj->next;
        free_obj(obj);
        obj = next;
    }
}
