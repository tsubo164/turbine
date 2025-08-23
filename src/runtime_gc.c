#include "runtime_gc.h"
#include "runtime_vec.h"
#include "runtime_map.h"
#include "runtime_set.h"
#include "runtime_stack.h"
#include "runtime_queue.h"
#include "runtime_string.h"
#include "runtime_struct.h"
#include "vm_cpu.h"
#include "os.h"

#include <assert.h>
#include <stdio.h>

enum gc_object_mark {
    MARK_WHITE,
    MARK_GRAY,
    MARK_BLACK,
};

enum gc_request_mode {
    REQ_NONE,
    REQ_AT_SAFEPOINT,
    REQ_FORCE_NOW,
};

enum gc_phase {
    PHASE_IDLE,
    PHASE_PREPARE,
    PHASE_MARK,
    PHASE_SWEEP,
    PHASE_FINISH,
};

#define INIT_THRESHOLD_MULT 1.5
#define INIT_THRESHOLD_BYTES (1 * 1024 * 1024) /* 1 Mbyte */
#define MAX_THRESHOLD_BYTES (128 * 1024 * 1024)

void runtime_gc_init(struct runtime_gc *gc)
{
    gc->used_bytes = 0;
    gc->threshold_bytes = INIT_THRESHOLD_BYTES;
    gc->max_threshold_bytes = MAX_THRESHOLD_BYTES;
    runtime_gc_set_threshold_multiplier(gc, INIT_THRESHOLD_MULT);

    /* work */
    runtime_gc_worklist_init(&gc->worklist);

    /* log */
    runtime_gc_log_init(&gc->log);

    gc->time_start = 0.;
    gc->time_end = 0.;
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

    /* work */
    runtime_gc_worklist_clear(&gc->worklist);

    /* log */
    runtime_gc_log_clear(&gc->log);
}

/* memory */
struct gc_header {
    size_t size;
};

static void check_heap_threshold(struct runtime_gc *gc)
{
    if (gc && gc->used_bytes >= gc->threshold_bytes) {
        runtime_gc_request_collect(gc);
        gc->trigger_reason = REASON_THRESHOLD;
    }
}

void *runtime_gc_alloc(struct runtime_gc *gc, size_t user_size)
{
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

/* object */
void *runtime_alloc_object(struct runtime_gc *gc, int kind, size_t size)
{
    static uint32_t id = 1;
    struct runtime_object *obj = runtime_gc_alloc(gc, size);

    /* TODO thread-safe */
    obj->kind = kind;
    obj->mark = MARK_BLACK;
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

uint32_t runtime_gc_get_object_id(const struct runtime_object *obj)
{
    if (!obj)
        return 0;
    return obj->id;
}

bool runtime_gc_is_object_alive(const struct runtime_gc *gc, value_int_t id)
{
    struct runtime_object *curr;

    for (curr = gc->root; curr; curr = curr->next) {
        if (curr->id == id)
            return true;
    }
    return false;
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

/* collect */
void runtime_gc_request_collect(struct runtime_gc *gc)
{
    if (gc->phase == PHASE_IDLE) {
        gc->phase = PHASE_PREPARE;
        gc->request_mode = REQ_AT_SAFEPOINT;
        gc->trigger_reason = REASON_USER;
    }
}

void runtime_gc_force_collect(struct runtime_gc *gc)
{
    gc->request_mode = REQ_FORCE_NOW;
    gc->trigger_reason = REASON_USER;
}

bool runtime_gc_is_requested(const struct runtime_gc *gc)
{
    return gc->request_mode != REQ_NONE;
}

bool runtime_gc_is_forced(const struct runtime_gc *gc)
{
    return gc->request_mode == REQ_FORCE_NOW;
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

static void push_to_worklist(struct runtime_gc *gc, struct runtime_object *obj)
{
    if (obj->mark != MARK_WHITE)
        return;
    obj->mark = MARK_GRAY;

    struct runtime_value val = {.obj = obj};
    runtime_gc_worklist_push(&gc->worklist, val);
}

static void scan_globals(struct runtime_gc *gc)
{
    int nglobals = vm_get_global_count(gc->vm);

    for (int i = 0; i < nglobals; i++) {
        bool is_ref = code_globalmap_is_ref(gc->globalmap, i);

        if (is_ref) {
            struct runtime_value val = vm_get_global(gc->vm, i);
            push_to_worklist(gc, val.obj);
        }
    }
}

static void scan_locals(struct runtime_gc *gc, value_addr_t inst_addr)
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

        for (int i = 0; i < nslots; i++) {
            bool is_ref = code_stackmap_is_ref(ent, i);

            if (is_ref) {
                value_addr_t bp = call->current_bp;
                struct runtime_value val = vm_lookup_stack(gc->vm, bp, i);
                push_to_worklist(gc, val.obj);
            }
        }

        callsite_addr = call->callsite_ip;
    }
}

static void scan_roots(struct runtime_gc *gc, value_addr_t inst_addr)
{
    scan_globals(gc);
    scan_locals(gc, inst_addr);
}

static void trace_refs(struct runtime_gc *gc, struct runtime_object *obj)
{
    switch ((enum runtime_object_kind) obj->kind) {

    case OBJ_NIL:
    case OBJ_STRING:
        break;

    case OBJ_VEC:
        {
            struct runtime_vec *v = (struct runtime_vec *) obj;

            if (is_ref_type(v->val_type)) {
                for (int i = 0; i < runtime_vec_len(v); i++) {
                    struct runtime_value val = runtime_vec_get(v, i);
                    push_to_worklist(gc, val.obj);
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
                    push_to_worklist(gc, val.obj);
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
                    push_to_worklist(gc, val.obj);
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
                    push_to_worklist(gc, val.obj);
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
                    push_to_worklist(gc, val.obj);
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
                    push_to_worklist(gc, val.obj);
                }
            }
        }
        break;
    }
}

static void drain_worklist(struct runtime_gc *gc)
{
    while (!runtime_gc_worklist_empty(&gc->worklist)) {
        struct runtime_value val = runtime_gc_worklist_pop(&gc->worklist);
        struct runtime_object *obj = val.obj;

        if (obj->mark == MARK_GRAY) {
            obj->mark = MARK_BLACK;
            trace_refs(gc, obj);
        }
    }
}

static void prepare(struct runtime_gc *gc, value_addr_t inst_addr)
{
    /* log */
    runtime_gc_log_init_entry(&gc->current_log_entry);

    gc->current_log_entry.triggered_addr = inst_addr;
    gc->current_log_entry.used_bytes_before = gc->used_bytes;
    gc->current_log_entry.trigger_reason = gc->trigger_reason;
    assert(gc->trigger_reason != REASON_NONE);

    /* time */
    gc->time_start = os_perf();
}

static void clear_marks(struct runtime_gc *gc)
{
    for (struct runtime_object *obj = gc->root; obj; obj = obj->next) {
        obj->mark = MARK_WHITE;
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
        if (curr->mark == MARK_WHITE) {
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

static void finish(struct runtime_gc *gc)
{
    gc->time_end = os_perf();

    /* update threshold bytes */
    gc->threshold_bytes *= gc->threshold_multiplier;
    if (gc->threshold_bytes > gc->max_threshold_bytes)
        gc->threshold_bytes = gc->max_threshold_bytes;

    gc->total_collections++;
    gc->request_mode = REQ_NONE;

    /* log */
    gc->current_log_entry.total_collections = gc->total_collections;
    gc->current_log_entry.used_bytes_after = gc->used_bytes;
    gc->current_log_entry.duration_msec = (gc->time_end - gc->time_start) * 1000.;
    runtime_gc_log_push(&gc->log, &gc->current_log_entry);
}

void runtime_gc_step(struct runtime_gc *gc, value_addr_t inst_addr)
{
    assert(inst_addr >= 0);

    switch ((enum gc_phase) gc->phase) {

    case PHASE_IDLE:
        /* does nothing and return */
        return;

    case PHASE_PREPARE:
        prepare(gc, inst_addr);
        clear_marks(gc);
        break;

    case PHASE_MARK:
        scan_roots(gc, inst_addr);
        drain_worklist(gc);
        break;

    case PHASE_SWEEP:
        free_unreachables(gc);
        break;

    case PHASE_FINISH:
        finish(gc);
        break;
    }

    if (gc->phase == PHASE_FINISH)
        gc->phase = PHASE_IDLE;
    else
        gc->phase++;
}

void runtime_gc_collect_objects(struct runtime_gc *gc, value_addr_t inst_addr)
{
    assert(inst_addr >= 0);

    if (gc->phase == PHASE_IDLE)
        gc->phase = PHASE_PREPARE;

    do {
        runtime_gc_step(gc, inst_addr);
    } while (gc->phase != PHASE_IDLE);
}

/* stats */
static const char *format_bytes(size_t bytes)
{
    static char buf[32] = {'\0'};

    if (bytes >= 1024 * 1024 * 1024) {
        snprintf(buf, sizeof(buf), "%.2f GB   ", (double)bytes / (1024 * 1024 * 1024));
    }
    else if (bytes >= 1024 * 1024) {
        snprintf(buf, sizeof(buf), "%.2f MB   ", (double)bytes / (1024 * 1024));
    }
    else if (bytes >= 1024) {
        snprintf(buf, sizeof(buf), "%.2f KB   ", (double)bytes / 1024);
    }
    else {
        snprintf(buf, sizeof(buf), "%zu bytes", bytes);
    }

    return buf;
}

void runtime_gc_print_stats(const struct runtime_gc *gc)
{
    double usage_percent = 0.0;
    if (gc->threshold_bytes > 0)
        usage_percent = (double)gc->used_bytes / gc->threshold_bytes;

    printf("GC status:\n");
    printf("  * usage:      %16s\n", format_bytes(gc->used_bytes));
    printf("  * threshold:  %16s\n", format_bytes(gc->threshold_bytes));
    printf("  * percentage: %10.2f %%\n", usage_percent * 100);
}

void runtime_gc_set_threshold_multiplier(struct runtime_gc *gc, float threshold_multiplier)
{
    float mult = threshold_multiplier;

    mult = mult > 3. ? 3. : mult;
    mult = mult < 1. ? 1. : mult;

    gc->threshold_multiplier = mult;
}

int runtime_gc_get_log_entry_count(const struct runtime_gc *gc)
{
    return runtime_gc_log_get_count(&gc->log);
}

const struct runtime_gc_log_entry *runtime_gc_get_log_entry(const struct runtime_gc *gc, int index)
{
    return runtime_gc_log_get_entry(&gc->log, index);
}
