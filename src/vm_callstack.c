#include "vm_callstack.h"
#include <stdlib.h>

#define MIN_CAP 16

void vm_callstack_init(struct vm_callstack *v)
{
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

void vm_callstack_free(struct vm_callstack *v)
{
    if (!v)
        return;
    free(v->data);
}

void vm_callstack_push(struct vm_callstack *v, const struct vm_call *call)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = *call;
}

void vm_callstack_pop(struct vm_callstack *v, struct vm_call *call)
{
    if (!v->data)
        return;
    *call = v->data[--v->len];
}

bool vm_callstack_is_empty(const struct vm_callstack *v)
{
    return v->len == 0;
}
