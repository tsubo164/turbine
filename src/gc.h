#ifndef GC_H
#define GC_H

enum ObjKind {
    OBJ_NIL,
    OBJ_STRING,
};

typedef struct Obj {
    struct Obj *next;
    int kind;
    bool marked;
} Obj;

typedef struct StringObj {
    Obj obj;
    const char *data;
} StringObj;

typedef struct GC {
    Obj *root;
} GC;

StringObj *NewString(GC *gc, const char *s);
void PrintObjs(const GC *gc);

#endif // _H
