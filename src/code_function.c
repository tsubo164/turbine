#include "code_function.h"
#include <stdlib.h>

/* TODO investigave why 16 fails test. */
#define MIN_CAP 8

void code_push_function(struct code_functionvec *v, int id, int argc, int64_t addr)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }

    struct code_function *func = &v->data[v->len++];
    func->id = id;
    func->argc = argc;
    func->addr = addr;
}

struct code_function *code_lookup_function(struct code_functionvec *v, int id)
{
    if (id < 0 || id >= v->len)
        return NULL;

    return &v->data[id];
}
