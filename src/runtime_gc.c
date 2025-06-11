#include "runtime_gc.h"
#include "runtime_vec.h"
#include "runtime_map.h"
#include "runtime_set.h"
#include "runtime_stack.h"
#include "runtime_queue.h"
#include "runtime_string.h"
#include "runtime_struct.h"
#include "vm_cpu.h"

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

void runtime_gc_collect_objects(struct runtime_gc *gc)
{
    /* clear marks */
    for (struct runtime_object *obj = gc->root; obj; obj = obj->next) {
        obj->mark = OBJ_WHITE;
    }

    /* TODO Add empty entry at the first map */
    /* track from roots */
    value_addr_t prev_addr = gc->vm->ip - 1;
    printf("current addr: [%6" PRIaddr "]\n", prev_addr);
    const struct code_stackmap_entry *ent = code_stackmap_find_entry(gc->stackmap, prev_addr);
    code_stackmap_print_entry(ent);

    int ncalls = vm_get_callstack_count(gc->vm);
    for (int i = 0; i < ncalls; i++) {
        const struct vm_call *call = vm_get_call(gc->vm, i);
        printf("func_index: %d\n", call->func_index);
        printf("argc:       %d\n", call->argc);
        printf("return_reg: %d\n", call->return_reg);
        printf("return_ip:  %lld\n", call->return_ip);
        printf("return_bp:  %lld\n", call->return_bp);
        printf("return_sp:  %lld\n", call->return_sp);
    }

    /* currnt call stack */
    for (int i = 0; i < 64; i++) {
        bool is_ref = code_stackmap_is_ref(ent, i);

        if (is_ref) {
            /* TODO temporary solution. need callstack for native call? */
            value_addr_t bp = /*gc->vm->bp*/1;
            struct runtime_value val = vm_lookup_stack(gc->vm, bp, i);
            /*
            print_obj(val.obj);
            */
            val.obj->mark = OBJ_BLACK;
        }
    }

    /* free white objects */
    struct runtime_object head = {0};
    struct runtime_object *prev = &head;
    struct runtime_object *curr = gc->root;
    struct runtime_object *next = NULL;
    prev->next = curr;

    while (curr) {
        if (curr->mark == OBJ_WHITE) {
            /*
            printf("freeing!!! => ");
            print_obj(curr);
            */
            next = curr->next;
            free_obj(curr);
            prev->next = curr = next;
        }
        else {
            prev = curr;
            curr = prev->next;
        }
    }

    gc->root = head.next;
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
