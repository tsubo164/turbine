#include "vec.h"
#include <stdlib.h>
#include <stdio.h>

void VecPush(struct Vec *v, void *data)
{
    if (!v)
        return;

    if (v->len >= v->cap) {
        v->cap = v->cap < 4 ? 4 : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(v->data[0]));
    }
    v->data[v->len++] = data;
}

void VecFree(struct Vec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

#define MIN_CAP 8

// integer stack
void PushInt(struct IntStack *v, int64_t val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

int64_t PopInt(struct IntStack *v)
{
    if (!v->data)
        return 0;
    return v->data[--v->len];
}

bool IsEmptyInt(const struct IntStack *v)
{
    return v->len == 0;
}
