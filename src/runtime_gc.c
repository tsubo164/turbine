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

#define INIT_THRESHOLD_MULT 1.5
//#define INIT_THRESHOLD_BYTES (64 * 1024)
#define INIT_THRESHOLD_BYTES (64 * 1)
void runtime_gc_init(struct runtime_gc *gc)
{
    gc->used_bytes = 0;
    gc->threshold_bytes = INIT_THRESHOLD_BYTES;
    runtime_gc_set_threshold_multiplier(gc, INIT_THRESHOLD_MULT);
}

static void free_obj(struct runtime_gc *gc, struct runtime_object *obj);
void runtime_gc_clear(struct runtime_gc *gc)
{
    struct runtime_object *obj = gc->root;
    struct runtime_object *next = NULL;

    while (obj) {
        next = obj->next;
        free_obj(gc, obj);
        obj = next;
    }
}

/* memory */
struct gc_header {
    size_t size;
};

static void check_heap_threshold(struct runtime_gc *gc)
{
    if (gc && gc->used_bytes >= gc->threshold_bytes) {
        /*
        gc->needs_collect = true;
        */
    }
}

void *runtime_gc_alloc(struct runtime_gc *gc, size_t user_size)
{
    /* */
    check_heap_threshold(gc);

    struct gc_header *header;

    header = calloc(1, sizeof(*header) + user_size);
    header->size = user_size;
    if (gc)
        gc->used_bytes += user_size;

    return header + 1;
}

void *runtime_gc_realloc(struct runtime_gc *gc, void *user_ptr, size_t user_size)
{
    struct gc_header *old_header = user_ptr;
    size_t old_size = 0;

    if (old_header) {
        old_header--;
        old_size = old_header->size;
    }

    struct gc_header *new_header;
    size_t new_size = user_size;

    new_header = realloc(old_header, sizeof(*new_header) + new_size);
    new_header->size = new_size;
    if (gc)
        gc->used_bytes += new_size - old_size;

    return new_header + 1;
}

void runtime_gc_free(struct runtime_gc *gc, void *user_ptr)
{
    if (!user_ptr)
        return;

    struct gc_header *header = user_ptr;
    header--;
    if (gc) {
        assert(gc->used_bytes >= header->size);
        gc->used_bytes -= header->size;
    }

    free(header);
}
/* -- */

void *runtime_alloc_object(struct runtime_gc *gc, int kind, size_t size)
{
    static uint32_t id = 1;
    struct runtime_object *obj = runtime_gc_alloc(gc, size);

    /* TODO thread-safe */
    obj->kind = kind;
    obj->id = id++;

    return obj;
}

void runtime_gc_push_object(struct runtime_gc *gc, struct runtime_object *obj)
{
    if (!gc)
        return;
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

    case OBJ_QUEUE:
        {
            const struct runtime_queue *q = (struct runtime_queue *) obj;
            printf("[%6" PRIu32 "] ", obj->id);
            printf("[%6s] => len: %" PRIival "\n", "queue", runtime_queue_len(q));
        }
        break;

    case OBJ_STRUCT:
        {
            const struct runtime_struct *s = (struct runtime_struct *) obj;
            printf("[%6" PRIu32 "] ", obj->id);
            printf("[%6s] => fields: %" PRIival "\n", "struct", runtime_struct_field_count(s));
        }
        break;
    }
}

void runtime_gc_print_objects(const struct runtime_gc *gc)
{
    printf("* %ld bytes used in heap:\n", gc->used_bytes);
    for (struct runtime_object *obj = gc->root; obj; obj = obj->next) {
        print_obj(obj);
    }
}

static void free_obj(struct runtime_gc *gc, struct runtime_object *obj)
{
    enum runtime_object_kind kind = obj->kind;

    switch (kind) {

    case OBJ_NIL:
        break;

    case OBJ_STRING:
        {
            struct runtime_string *s = (struct runtime_string *) obj;
            runtime_string_free(gc, s);
        }
        break;

    case OBJ_VEC:
        {
            struct runtime_vec *v = (struct runtime_vec *) obj;
            runtime_vec_free(gc, v);
        }
        break;

    case OBJ_MAP:
        {
            struct runtime_map *m = (struct runtime_map *) obj;
            runtime_map_free(gc, m);
        }
        break;

    case OBJ_SET:
        {
            struct runtime_set *s = (struct runtime_set *) obj;
            runtime_set_free(gc, s);
        }
        break;

    case OBJ_STACK:
        {
            struct runtime_stack *s = (struct runtime_stack *) obj;
            runtime_stack_free(gc, s);
        }
        break;

    case OBJ_QUEUE:
        {
            struct runtime_queue *q = (struct runtime_queue *) obj;
            runtime_queue_free(gc, q);
        }
        break;

    case OBJ_STRUCT:
        {
            struct runtime_struct *s = (struct runtime_struct *) obj;
            runtime_struct_free(gc, s);
        }
        break;
    }
}

void runtime_gc_request_collect(struct runtime_gc *gc)
{
    gc->needs_collect = true;
}

bool runtime_gc_is_requested(const struct runtime_gc *gc)
{
    return gc->needs_collect;
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

static void mark_object(struct runtime_gc *gc, struct runtime_object *obj)
{
    /* 'obj' may be NULL between struct creation and initialization.
     * A safe point could be added later to allow assert(obj); */
    if (!obj)
        return;

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
                    mark_object(gc, val.obj);
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
                    mark_object(gc, val.obj);
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
                    mark_object(gc, val.obj);
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
                    mark_object(gc, val.obj);
                }
            }
        }
        break;

    case OBJ_QUEUE:
        {
            struct runtime_queue *q = (struct runtime_queue *) obj;

            if (is_ref_type(q->val_type)) {
                int len = runtime_queue_len(q);
                for (int i = 0; i < len; i++) {
                    struct runtime_value val = runtime_queue_get(q, i);
                    mark_object(gc, val.obj);
                }
            }
        }
        break;

    case OBJ_STRUCT:
        {
            struct runtime_struct *s = (struct runtime_struct *) obj;
            int struct_id = s->id;
            int len = runtime_struct_field_count(s);

            for (int i = 0; i < len; i++) {
                int val_type = code_get_struct_field_type(gc->vm->code, struct_id, i);

                if (is_ref_type(val_type)) {
                    struct runtime_value val = runtime_struct_get(s, i);
                    mark_object(gc, val.obj);
                }
            }
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
            mark_object(gc, val.obj);
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

        const struct code_stackmap_entry *ent = code_stackmap_find_entry(gc->stackmap, callsite_addr);
        const struct vm_call *call = vm_get_call(gc->vm, frame_id);
        int nslots = call->current_sp - call->current_bp;
        assert(nslots <= 64);

        if (0) {
            printf("-------------------------------------\n");
            vm_print_call(call);
            code_print_stackmap(gc->stackmap);
            //printf("======================> callsite_addr: %lld\n", callsite_addr);
        }

        for (int i = 0; i < nslots; i++) {
            bool is_ref = code_stackmap_is_ref(ent, i);

            if (is_ref) {
                value_addr_t bp = call->current_bp;
                //printf("====================== callsite_addr: %lld, bp: %lld, i: %d\n", callsite_addr, bp, i);
                struct runtime_value val = vm_lookup_stack(gc->vm, bp, i);
                mark_object(gc, val.obj);
                //print_obj(val.obj);
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
            free_obj(gc, curr);
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
    /* TODO */
    if (inst_addr == 0)
        return;
    /*
    printf("----------------------------------------- GC!!!! @ %lld\n", inst_addr);
    runtime_gc_print_objects(gc);
    */

    /* clear marks */
    clear_marks(gc);

    /* trace globals */
    trace_globals(gc);

    /* trace locals and temps */
    trace_locals(gc, inst_addr);

    /* free white objects */
    free_unreachables(gc);

    gc->needs_collect = false;

    /*
    printf("-----------------------------------------GC done\n");
    runtime_gc_print_objects(gc);
    */
}

void runtime_gc_set_threshold_multiplier(struct runtime_gc *gc, float threshold_multiplier)
{
    float mult = threshold_multiplier;

    mult = mult > 3. ? 3. : mult;
    mult = mult < 1. ? 1. : mult;

    gc->threshold_multiplier = mult;
}
