#include "data_vec.h"
#include <stdlib.h>
#include <stdio.h>

#define MIN_CAP 8

/* Vector of int */
void data_intvec_init(struct data_intvec *v)
{
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

bool data_intvec_is_empty(const struct data_intvec *v)
{
    return v->len == 0;
}

void data_intvec_resize(struct data_intvec *v, int new_len)
{
    if (new_len <= v->cap) {
        v->len = new_len;
        return;
    }

    int new_cap = v->cap < (MIN_CAP) ? (MIN_CAP) : v->cap;
    while (new_cap < new_len)
        new_cap *= 2;

    v->data = realloc(v->data, sizeof(*v->data) * new_cap);
    v->cap = new_cap;
    v->len = new_len;
}

void data_intvec_push(struct data_intvec *v, value_int_t val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

void data_intvec_free(struct data_intvec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

/* Stack of int */
void data_intstack_push(struct data_intstack *v, value_int_t val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

value_int_t data_intstack_pop(struct data_intstack *v)
{
    if (!v->data)
        return 0;
    return v->data[--v->len];
}

value_int_t data_intstack_top(const struct data_intstack *v)
{
    if (!v->data)
        return 0;
    return v->data[v->len - 1];
}

bool data_intstack_is_empty(const struct data_intstack *v)
{
    return v->len == 0;
}

void data_intstack_free(struct data_intstack *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}
