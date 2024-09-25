#ifndef GC_H
#define GC_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t  Byte;
typedef uint16_t Word;
typedef int64_t  Int;
typedef double   Float;

struct StringObj;

// Value
typedef struct Value {
    union {
        Int inum;
        Float fpnum;
        struct StringObj *str;
        struct ObjArray *array;
    };
} Value;

// Value vecor
typedef struct ValueVec {
    Value *data;
    int cap;
    int len;
} ValueVec;

void ValueVecReisze(struct ValueVec *v, int new_len);
void ValueVecPush(struct ValueVec *v, Value val);

// GC Object
enum ObjKind {
    OBJ_NIL,
    OBJ_STRING,
    OBJ_ARRAY,
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
void PrintObjects(const GC *gc);

#endif // _H
