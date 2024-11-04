#include "value.h"
#include "data_vec.h"
#include <stdlib.h>
#include <string.h>

#define MIN_CAP 8

void ValueVecInit(struct ValueVec *v)
{
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

bool ValueVecIsEmpty(const struct ValueVec *v)
{
    return v->len == 0;
}

void ValueVecResize(struct ValueVec *v, int new_len)
{
    if (new_len <= v->cap) {
        v->len = new_len;
        return;
    }

    int new_cap = v->cap < MIN_CAP ? MIN_CAP : v->cap;
    while (new_cap < new_len)
        new_cap *= 2;

    v->data = realloc(v->data, sizeof(*v->data) * new_cap);
    v->cap = new_cap;
    v->len = new_len;
}

void ValueVecPush(struct ValueVec *v, struct Value val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

struct Value ValueVecGet(const struct ValueVec *v, int index)
{
    if (index < 0 || index >= v->len) {
        struct Value val = {0};
        return val;
    }
    return v->data[index];
}

void ValueVecFree(struct ValueVec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

void runtime_valuevec_zeroclear(struct ValueVec *v)
{
    memset(v->data, 0, v->len * sizeof(*v->data));
}
