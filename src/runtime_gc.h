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

struct runtime_string;

struct runtime_string *runtime_gc_new_string(struct runtime_gc *gc, const char *cstr);

void runtime_gc_push_object(struct runtime_gc *gc, struct runtime_object *obj);
void runtime_gc_print_objects(const struct runtime_gc *gc);

#endif /* _H */
