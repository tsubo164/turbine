#ifndef GC_H
#define GC_H

#include "value.h"

// GC Object
enum ObjKind {
    OBJ_NIL,
    OBJ_STRING,
    OBJ_ARRAY_,
    OBJ_ARRAY,
    OBJ_STRUCT,
};

typedef struct Obj {
    struct Obj *next;
    int kind;
    bool marked;
} Obj;

typedef struct StringObj {
    Obj obj;
    char *data;
    int len;
} StringObj;

typedef struct GC {
    Obj *root;
} GC;

StringObj *NewString(GC *gc, const char *s);

struct StringObj *GCStringNew(const char *s);
void GCStringFree(struct StringObj *str);

void runtime_append_gc_object(struct GC *gc, struct Obj *obj);

void PrintObjects(const GC *gc);

#endif // _H
