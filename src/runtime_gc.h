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
    bool need_collect;
};

/* memory */
void *runtime_gc_alloc(struct runtime_gc *gc, size_t user_size);
void *runtime_gc_realloc(struct runtime_gc *gc, void *user_ptr, size_t user_size);
void runtime_gc_free(struct runtime_gc *gc, void *user_ptr);

/* alloc for vec, map, set, stack and queue */
void *runtime_alloc_object(int kind, size_t size);
void *runtime_alloc_object2(struct runtime_gc *gc, int kind, size_t size);

/* objects */
void runtime_gc_push_object(struct runtime_gc *gc, struct runtime_object *obj);
void runtime_gc_print_objects(const struct runtime_gc *gc);
void runtime_gc_request_collect(struct runtime_gc *gc);
bool runtime_gc_is_requested(const struct runtime_gc *gc);
void runtime_gc_collect_objects(struct runtime_gc *gc, value_addr_t inst_addr);

void runtime_gc_clear(struct runtime_gc *gc);

#endif /* _H */
