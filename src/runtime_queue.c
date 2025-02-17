#include "runtime_queue.h"
#include <stdlib.h>
#include <stdio.h>

struct runtime_queue *runtime_queue_new(int val_type, int64_t len)
{
    struct runtime_queue *q;

    q = calloc(1, sizeof(*q));
    q->obj.kind = OBJ_QUEUE;
    q->val_type = val_type;
    q->compare = runtime_get_compare_function(q->val_type);

    return q;
}

void runtime_queue_free(struct runtime_queue *q)
{
    if (!q)
        return;
    free(q->data);
    free(q);
}

int64_t runtime_queue_len(const struct runtime_queue *q)
{
    return q->len;
}

bool runtime_queue_empty(const struct runtime_queue *q)
{
    return runtime_queue_len(q) == 0;
}

#define MIN_CAP 8

static void expand(struct runtime_queue *q)
{
    struct runtime_value *new_data;
    int new_cap = q->cap < MIN_CAP ? MIN_CAP : q->cap * 2;

    new_data = malloc(sizeof(*new_data) * new_cap);
    for (int i = 0; i < q->len; i++) {
        new_data[i] = q->data[(q->front + i) % q->cap];
    }

    free(q->data);
    q->data = new_data;
    q->cap = new_cap;
    q->front = 0;
    q->back = q->len;
}

void runtime_queue_push(struct runtime_queue *q, struct runtime_value val)
{
    if (q->len == q->cap)
        expand(q);

    q->data[q->back] = val;
    q->back = (q->back + 1) % q->cap;
    q->len++;
}

struct runtime_value runtime_queue_pop(struct runtime_queue *q)
{
    struct runtime_value val = {0};

    if (runtime_queue_empty(q))
        return val;

    val = q->data[q->front];
    q->front = (q->front + 1) % q->cap;
    q->len--;
    return val;
}

struct runtime_value runtime_queue_front(const struct runtime_queue *q)
{
    struct runtime_value val = {0};

    if (runtime_queue_empty(q))
        return val;

    return q->data[q->front];
}

/* No index range check */
struct runtime_value runtime_queue_get(const struct runtime_queue *q, int64_t idx)
{
    int64_t i = (q->front + idx) % q->cap;
    return q->data[i];
}
