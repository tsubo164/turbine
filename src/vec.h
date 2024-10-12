#ifndef VEC_H
#define VEC_H

struct Vec {
    void **data;
    int cap;
    int len;
};

void VecPush(struct Vec *v, void *data);
void VecFree(struct Vec *v);

#include <stdlib.h>

/*
// Struct definition and its function declaratios of dynamic array and stack
// The macros to define the functions with struct name e.g. IntVec
// Example.

// vector of int
struct IntVec {
int *data;
int cap;
int len;
};

// For vector and stack.
void IntVecInit(struct IntVec *v);
bool IntVecIsEmpty(const struct IntVec *v);
void IntVecResize(struct IntVec *v, int new_len);
void IntVecPush(struct IntVec *v, int val);
void IntVecFree(struct IntVec *v);

// For stack. No empty checking
int IntStackTop(const IntStack *s);
int IntStackPop(IntStack *s);
*/

// vector and stack functions
#define DEFINE_VECTOR_FUNCTIONS(type, name, mincap) \
void name##Init(struct name *v) \
{ \
    v->data = NULL; \
    v->cap = 0; \
    v->len = 0; \
} \
bool name##IsEmpty(const struct name *v) \
{ \
    return v->len == 0; \
} \
void name##Resize(struct name *v, int new_len) \
{ \
    if (new_len <= v->cap) { \
        v->len = new_len; \
        return; \
    } \
    int new_cap = v->cap < (mincap) ? (mincap) : v->cap; \
    while (new_cap < new_len) \
        new_cap *= 2; \
    type *new_data = (type*) realloc(v->data, sizeof(*v->data) * new_cap); \
    if (!new_data) { \
        return; \
    } \
    v->data = new_data; \
    v->cap = new_cap; \
    v->len = new_len; \
} \
void name##Push(struct name *v, type val) \
{ \
    name##Resize(v, v->len + 1); \
    v->data[v->len - 1] = val; \
} \
void name##Free(struct name *v) \
{ \
    free(v->data); \
    name##Init(v); \
}

// stack functions
#define DEFINE_STACK_FUNCTIONS(type, name) \
    type name##Top(const struct name *v) \
{ \
    return v->data[v->len-1]; \
} \
type name##Pop(struct name *v) \
{ \
    return v->data[--v->len]; \
}

#endif // _H
