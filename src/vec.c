#include "vec.h"
#include <stdlib.h>
#include <stdio.h>

void VecPush(struct Vec *v, void *data)
{
    if (!v || !data)
        return;

    if (v->len >= v->cap) {
        v->cap = v->cap < 4 ? 4 : 2 * v->cap;
        v->data = realloc(v->data, v->cap);
    }
    v->data[v->len++] = data;
}
