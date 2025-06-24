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

    bool need_collect;
};

/* alloc for vec, map, set, stack and queue */
void *runtime_alloc_object(int kind, size_t size);

struct runtime_string;

struct runtime_string *runtime_gc_string_new(struct runtime_gc *gc, const char *cstr);

void runtime_gc_push_object(struct runtime_gc *gc, struct runtime_object *obj);
void runtime_gc_print_objects(const struct runtime_gc *gc);
void runtime_gc_request_collect(struct runtime_gc *gc);
bool runtime_gc_is_requested(const struct runtime_gc *gc);
void runtime_gc_collect_objects(struct runtime_gc *gc, value_addr_t inst_addr);

void runtime_gc_free(struct runtime_gc *gc);

#endif /* _H */
