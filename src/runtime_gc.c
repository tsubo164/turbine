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

void *runtime_alloc_object(int kind, size_t size)
{
    static uint32_t id = 1;
    struct runtime_object *obj = calloc(1, size);

    /* TODO thread-safe */
    obj->kind = kind;
    obj->id = id++;

    return obj;
}

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
            printf("[%6" PRIu32 "] ", obj->id);
            printf("[%s] => len: %d \"%s\"\n", "string", s->len, runtime_string_get_cstr(s));
        }
        break;

    case OBJ_VEC:
        {
            const struct runtime_vec *v = (struct runtime_vec *) obj;
            printf("[%6" PRIu32 "] ", obj->id);
            printf("[%6s] => len: %d, cap: %d\n", "vec", v->values.len, v->values.cap);
        }
        break;

    case OBJ_MAP:
        {
            const struct runtime_map *m = (struct runtime_map *) obj;
            printf("[%6" PRIu32 "] ", obj->id);
            printf("[%6s] => len: %" PRIival ", cap: %" PRIival "\n", "map", m->len, m->cap);
        }
        break;

    case OBJ_SET:
        {
            const struct runtime_set *s = (struct runtime_set *) obj;
            printf("[%6" PRIu32 "] ", obj->id);
            printf("[%6s] => len: %" PRIival "\n", "set", s->len);
        }
        break;

    case OBJ_STACK:
        {
            const struct runtime_stack *s = (struct runtime_stack *) obj;
            printf("[%6" PRIu32 "] ", obj->id);
            printf("[%6s] => len: %" PRIival "\n", "stack", runtime_stack_len(s));
        }
        break;

    case OBJ_STRUCT:
        {
            const struct runtime_struct *s = (struct runtime_struct *) obj;
            printf("[%6" PRIu32 "] ", obj->id);
            printf("[%6s] => fields: %" PRIival "\n", "struct", runtime_struct_field_count(s));
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

void runtime_gc_request_collect(struct runtime_gc *gc)
{
    gc->need_collect = true;
}

bool runtime_gc_is_requested(const struct runtime_gc *gc)
{
    return gc->need_collect;
}

static bool is_ref_type(int type)
{
    switch ((enum runtime_value_type) type) {
    case VAL_NIL:
    case VAL_INT:
    case VAL_FLOAT:
        return false;
    case VAL_STRING:
    case VAL_VEC:
    case VAL_MAP:
    case VAL_SET:
    case VAL_STACK:
    case VAL_QUEUE:
    case VAL_STRUCT:
        return true;
    }

    return false;
}

static void mark_object(struct runtime_object *obj)
{
    /* mark this object */
    obj->mark = OBJ_BLACK;

    enum runtime_object_kind kind = obj->kind;

    switch (kind) {

    case OBJ_NIL:
        break;

    case OBJ_STRING:
        break;

    case OBJ_VEC:
        {
            struct runtime_vec *v = (struct runtime_vec *) obj;

            if (is_ref_type(v->val_type)) {
                for (int i = 0; i < runtime_vec_len(v); i++) {
                    struct runtime_value val = runtime_vec_get(v, i);
                    mark_object(val.obj);
                }
            }
        }
        break;

    case OBJ_MAP:
        {
            struct runtime_map *m = (struct runtime_map *) obj;

            if (is_ref_type(m->val_type)) {
                struct runtime_map_entry *ent = runtime_map_entry_begin(m);
                for (; ent; ent = runtime_map_entry_next(ent)) {
                    struct runtime_value val = ent->val;
                    mark_object(val.obj);
                }
            }
        }
        break;

    case OBJ_SET:
        {
            struct runtime_set *s = (struct runtime_set *) obj;

            if (is_ref_type(s->val_type)) {
                struct runtime_set_node *node = runtime_set_node_begin(s);
                for (; node; node = runtime_set_node_next(node)) {
                    struct runtime_value val = node->val;
                    mark_object(val.obj);
                }
            }
        }
        break;

    case OBJ_STACK:
        {
            struct runtime_stack *s = (struct runtime_stack *) obj;

            if (is_ref_type(s->val_type)) {
                int len = runtime_stack_len(s);
                for (int i = 0; i < len; i++) {
                    struct runtime_value val = runtime_stack_get(s, i);
                    mark_object(val.obj);
                }
            }
        }
        break;

    case OBJ_QUEUE:
        {
            //struct runtime_queue *q = (struct runtime_queue *) obj;
        }
        break;

    case OBJ_STRUCT:
        {
            //struct runtime_struct *s = (struct runtime_struct *) obj;
        }
        break;
    }
}

static void clear_marks(struct runtime_gc *gc)
{
    for (struct runtime_object *obj = gc->root; obj; obj = obj->next) {
        obj->mark = OBJ_WHITE;
    }
}

static void trace_globals(struct runtime_gc *gc)
{
    int nglobals = vm_get_global_count(gc->vm);

    for (int i = 0; i < nglobals; i++) {
        bool is_ref = code_globalmap_is_ref(gc->globalmap, i);
        if (is_ref) {
            struct runtime_value val = vm_get_global(gc->vm, i);
            mark_object(val.obj);
        }
    }
}

static void trace_locals(struct runtime_gc *gc, value_addr_t inst_addr)
{
    int ncalls = vm_get_callstack_count(gc->vm);
    value_addr_t callsite_addr = inst_addr;

    for (int frame_id = ncalls - 1; frame_id >= 0; frame_id--) {
        /* no object at the beginning */
        if (callsite_addr == 0)
            break;

        value_addr_t precall_addr = callsite_addr - 1;
        const struct code_stackmap_entry *ent = code_stackmap_find_entry(gc->stackmap, precall_addr);
        const struct vm_call *call = vm_get_call(gc->vm, frame_id);
        int nslots = call->current_sp - call->current_bp;
        assert(nslots <= 64);

        if (0) {
            printf("-------------------------------------\n");
            vm_print_call(call);
        }

        for (int i = 0; i < nslots; i++) {
            bool is_ref = code_stackmap_is_ref(ent, i);

            if (is_ref) {
                value_addr_t bp = call->current_bp;
                struct runtime_value val = vm_lookup_stack(gc->vm, bp, i);
                mark_object(val.obj);
            }
        }

        callsite_addr = call->callsite_ip;
    }
}

static void free_unreachables(struct runtime_gc *gc)
{
    struct runtime_object head = {0};
    struct runtime_object *prev = &head;
    struct runtime_object *curr = gc->root;
    struct runtime_object *next = NULL;
    prev->next = curr;

    while (curr) {
        if (curr->mark == OBJ_WHITE) {
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

void runtime_gc_collect_objects(struct runtime_gc *gc, value_addr_t inst_addr)
{
    /* clear marks */
    clear_marks(gc);

    /* trace globals */
    trace_globals(gc);

    /* trace locals and temps */
    trace_locals(gc, inst_addr);

    /* free white objects */
    free_unreachables(gc);

    gc->need_collect = false;
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
