#include "runtime_value.h"
#include <stdlib.h>
#include <string.h>

#define MIN_CAP 8

void runtime_valuevec_init(struct runtime_valuevec *v)
{
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

bool runtime_valuevec_is_empty(const struct runtime_valuevec *v)
{
    return v->len == 0;
}

int runtime_valuevec_len(const struct runtime_valuevec *v)
{
    return v->len;
}

void runtime_valuevec_resize(struct runtime_valuevec *v, int new_len)
{
    if (new_len <= v->cap) {
        v->len = new_len;
        return;
    }

    int new_cap = v->cap < MIN_CAP ? MIN_CAP : v->cap;
    while (new_cap < new_len)
        new_cap *= 2;

    int old_len = v->len;
    v->data = realloc(v->data, sizeof(*v->data) * new_cap);
    v->cap = new_cap;
    v->len = new_len;

    memset(v->data + old_len, 0, (new_len - old_len) * sizeof(*v->data));
}

void runtime_valuevec_push(struct runtime_valuevec *v, struct runtime_value val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

struct runtime_value runtime_valuevec_get(const struct runtime_valuevec *v, int idx)
{
    if (idx < 0 || idx >= v->len) {
        struct runtime_value val = {0};
        return val;
    }
    return v->data[idx];
}

void runtime_valuevec_set(struct runtime_valuevec *v, int idx, struct runtime_value val)
{
    if (idx < 0 || idx >= v->len) {
        return;
    }
    v->data[idx] = val;
}

void runtime_valuevec_free(struct runtime_valuevec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}
