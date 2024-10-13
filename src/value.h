#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t  Byte;
typedef uint16_t Word;
typedef int64_t  Int;
typedef double   Float;

struct StringObj;
struct ObjArray;
struct GCArray;

// Value type
enum ValueType {
    VAL_NIL = 0,
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
};

// Value
typedef struct Value {
    union {
        Int inum;
        Float fpnum;
        struct StringObj *str;
        struct ObjArray *array_; // OLD
        struct GCArray *array;
    };
} Value;

// Vecor of value
struct ValueVec {
    struct Value *data;
    int cap;
    int len;
};

void ValueVecInit(struct ValueVec *v);
bool ValueVecIsEmpty(const struct ValueVec *v);
void ValueVecResize(struct ValueVec *v, int new_len);
void ValueVecPush(struct ValueVec *v, struct Value val);
void ValueVecFree(struct ValueVec *v);

#endif // _H
