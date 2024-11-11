#ifndef GC_H
#define GC_H

#include "runtime_value.h"

// GC Object
enum runtime_object_kind {
    OBJ_NIL,
    OBJ_STRING,
    OBJ_ARRAY_,
    OBJ_ARRAY,
    OBJ_STRUCT,
};

struct runtime_object {
    struct runtime_object *next;
    int kind;
    bool marked;
};

/*
typedef struct StringObj {
    Obj obj;
    char *data;
    int len;
} StringObj;
*/

typedef struct GC {
    struct runtime_object *root;
} GC;

/*
StringObj *NewString(GC *gc, const char *s);

struct StringObj *GCStringNew(const char *s);
void GCStringFree(struct StringObj *str);
int runtime_string_compare_cstr(const struct StringObj *str, const char *cstr);
int runtime_string_compare(const struct StringObj *a, const struct StringObj *b);
struct StringObj *runtime_string_concat(const struct StringObj *a, const struct StringObj *b);
const char *runtime_string_get_cstr(const struct StringObj *s);
*/

void runtime_append_gc_object(struct GC *gc, struct runtime_object *obj);

void PrintObjects(const GC *gc);

#endif // _H
