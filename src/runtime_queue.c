#include "runtime_queue.h"
#include <stdlib.h>
#include <stdio.h>

struct runtime_queue *runtime_queue_new(struct runtime_gc *gc, int val_type, value_int_t len)
{
    struct runtime_queue *q;

    q = runtime_alloc_object(gc, OBJ_QUEUE, sizeof(*q));
    q->val_type = val_type;

    runtime_gc_push_object(gc, (struct runtime_object*) q);
    return q;
}

void runtime_queue_free(struct runtime_gc *gc, struct runtime_queue *q)
{
    if (!q)
        return;
    runtime_gc_free(gc, q->data);
    runtime_gc_free(gc, q);
}

value_int_t runtime_queue_len(const struct runtime_queue *q)
{
    return q->len;
}

bool runtime_queue_empty(const struct runtime_queue *q)
{
    return runtime_queue_len(q) == 0;
}

struct runtime_value runtime_queue_front(const struct runtime_queue *q)
{
    struct runtime_value val = {0};

    if (runtime_queue_empty(q))
        return val;

    return q->data[q->front];
}

#define MIN_CAP 8

static void expand(struct runtime_gc *gc, struct runtime_queue *q)
{
    struct runtime_value *new_data;
    int new_cap = q->cap < MIN_CAP ? MIN_CAP : q->cap * 2;

    new_data = runtime_gc_alloc(gc, new_cap * sizeof(*new_data));
    for (int i = 0; i < q->len; i++) {
        new_data[i] = q->data[(q->front + i) % q->cap];
    }

    runtime_gc_free(gc, q->data);
    q->data = new_data;
    q->cap = new_cap;
    q->front = 0;
    q->back = q->len;
}

/* gc managed */
void runtime_queue_push(struct runtime_gc *gc, struct runtime_queue *q, struct runtime_value val)
{
    if (q->len == q->cap)
        expand(gc, q);

    q->data[q->back] = val;
    q->back = (q->back + 1) % q->cap;
    q->len++;
}

struct runtime_value runtime_queue_pop(struct runtime_gc *gc, struct runtime_queue *q)
{
    struct runtime_value val = {0};

    if (runtime_queue_empty(q))
        return val;

    val = q->data[q->front];
    q->front = (q->front + 1) % q->cap;
    q->len--;
    return val;
}

/* No index range check */
struct runtime_value runtime_queue_get(const struct runtime_queue *q, value_int_t idx)
{
    value_int_t i = (q->front + idx) % q->cap;
    return q->data[i];
}
