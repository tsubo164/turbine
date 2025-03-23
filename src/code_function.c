#include "code_function.h"
#include <stdlib.h>

#define MIN_CAP 8

int code_push_function(struct code_functionvec *v, const char *fullname, int argc)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }

    int new_id = v->len++;
    struct code_function init = {0};
    struct code_function *func = &v->data[new_id];

    *func = init;
    func->id = new_id;
    func->argc = argc;
    func->addr = -1;
    func->fullname = fullname;

    return new_id;
}

struct code_function *code_lookup_function(struct code_functionvec *v, int id)
{
    if (id < 0 || id >= v->len)
        return NULL;

    return &v->data[id];
}

const struct code_function *code_lookup_const_function(const struct code_functionvec *v,
        int id)
{
    if (id < 0 || id >= v->len)
        return NULL;

    return &v->data[id];
}

void code_functionvec_free(struct code_functionvec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}
