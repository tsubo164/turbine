#ifndef RUNTIME_GC_H
#define RUNTIME_GC_H

#include "value_types.h"
#include <stdbool.h>
#include <stdlib.h>

enum runtime_object_kind {
    OBJ_NIL,
    OBJ_STRING,
    OBJ_VEC,
    OBJ_MAP,
    OBJ_SET,
    OBJ_STACK,
    OBJ_QUEUE,
    OBJ_STRUCT,
};

struct runtime_object {
    uint32_t id;
    int kind;
    int mark;
    struct runtime_object *next;
};

struct code_stackmap;
struct vm_cpu;

struct runtime_gc {
    struct runtime_object *root;

    const struct code_stackmap *stackmap;
    const struct code_globalmap *globalmap;
    const struct vm_cpu *vm;

    size_t used_bytes;
    size_t threshold_bytes;
    size_t max_threshold_bytes;
    float threshold_multiplier;
    bool needs_collect;

    int total_collections;
};

void runtime_gc_init(struct runtime_gc *gc);
void runtime_gc_clear(struct runtime_gc *gc);

/* memory */
void *runtime_gc_alloc(struct runtime_gc *gc, size_t user_size);
void *runtime_gc_realloc(struct runtime_gc *gc, void *user_ptr, size_t user_size);
void runtime_gc_free(struct runtime_gc *gc, void *user_ptr);

/* object */
void *runtime_alloc_object(struct runtime_gc *gc, int kind, size_t size);
void runtime_gc_push_object(struct runtime_gc *gc, struct runtime_object *obj);
uint32_t runtime_gc_get_object_id(const struct runtime_object *obj);
bool runtime_gc_is_object_alive(const struct runtime_gc *gc, value_int_t id);
void runtime_gc_print_objects(const struct runtime_gc *gc);

/* collect */
void runtime_gc_request_collect(struct runtime_gc *gc);
bool runtime_gc_is_requested(const struct runtime_gc *gc);
void runtime_gc_collect_objects(struct runtime_gc *gc, value_addr_t inst_addr);

/* stats */
void runtime_gc_print_stats(const struct runtime_gc *gc);
void runtime_gc_set_threshold_multiplier(struct runtime_gc *gc, float threshold_multiplier);

#endif /* _H */
