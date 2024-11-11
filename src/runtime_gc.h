#ifndef RUNTIME_GC_H
#define RUNTIME_GC_H

#include <stdbool.h>

enum runtime_object_kind {
    OBJ_NIL,
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_STRUCT,
};

struct runtime_object {
    int kind;
    bool marked;
    struct runtime_object *next;
};

struct runtime_gc {
    struct runtime_object *root;
};

void runtime_push_gc_object(struct runtime_gc *gc, struct runtime_object *obj);

void runtime_print_gc_objects(const struct runtime_gc *gc);

#endif /* _H */
