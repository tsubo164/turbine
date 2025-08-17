#include "runtime_gc_worklist.h"
#include <stdlib.h>
#include <stdio.h>

void runtime_gc_worklist_init(struct runtime_gc_worklist *list)
{
    struct runtime_gc_worklist init = {0};
    *list = init;
}

void runtime_gc_worklist_clear(struct runtime_gc_worklist *list)
{
    if (!list)
        return;
    free(list->data);
    runtime_gc_worklist_init(list);
}

value_int_t runtime_gc_worklist_len(const struct runtime_gc_worklist *list)
{
    return list->len;
}

bool runtime_gc_worklist_empty(const struct runtime_gc_worklist *list)
{
    return runtime_gc_worklist_len(list) == 0;
}

struct runtime_value runtime_gc_worklist_front(const struct runtime_gc_worklist *list)
{
    if (runtime_gc_worklist_empty(list)) {
        struct runtime_value val = {0};
        return val;
    }
    return list->data[list->front];
}

#define MIN_CAP 8

static void expand(struct runtime_gc_worklist *list)
{
    struct runtime_value *new_data;
    int new_cap = list->cap < MIN_CAP ? MIN_CAP : list->cap * 2;

    new_data = calloc(new_cap, sizeof(*new_data));
    for (int i = 0; i < list->len; i++) {
        new_data[i] = list->data[(list->front + i) % list->cap];
    }

    free(list->data);
    list->data = new_data;
    list->cap = new_cap;
    list->front = 0;
    list->back = list->len;
}

void runtime_gc_worklist_push(struct runtime_gc_worklist *list, struct runtime_value val)
{
    if (list->len == list->cap)
        expand(list);

    list->data[list->back] = val;
    list->back = (list->back + 1) % list->cap;
    list->len++;
}

struct runtime_value runtime_gc_worklist_pop(struct runtime_gc_worklist *list)
{
    struct runtime_value val = {0};

    if (runtime_gc_worklist_empty(list))
        return val;

    val = list->data[list->front];
    list->front = (list->front + 1) % list->cap;
    list->len--;
    return val;
}
