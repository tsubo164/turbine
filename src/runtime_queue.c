#include "runtime_queue.h"
#include <stdlib.h>
#include <stdio.h>

struct runtime_queue *runtime_queue_new(int val_type, int64_t len)
{
    struct runtime_queue *s;

    s = calloc(1, sizeof(*s));
    s->obj.kind = OBJ_QUEUE;
    s->val_type = val_type;
    s->compare = runtime_get_compare_function(s->val_type);

    return s;
}

void runtime_queue_free(struct runtime_queue *s)
{
    if (!s)
        return;
    runtime_valuevec_free(&s->values);
    free(s);
}

/*
int64_t runtime_queue_len(const struct runtime_queue *s)
{
    return runtime_valuevec_len(&s->values);
}

bool runtime_queue_empty(const struct runtime_queue *s)
{
    return runtime_valuevec_len(&s->values) == 0;
}

void runtime_queue_push(struct runtime_queue *s, struct runtime_value val)
{
    runtime_valuevec_push(&s->values, val);
}

struct runtime_value runtime_queue_pop(struct runtime_queue *s)
{
    struct runtime_valuevec *values = &s->values;
    struct runtime_value val = {0};

    if (runtime_queue_empty(s))
        return val;

    int len = runtime_valuevec_len(values);
    val = runtime_valuevec_get(values, len - 1);
    runtime_valuevec_resize(values, len - 1);

    return val;
}

struct runtime_value runtime_queue_top(const struct runtime_queue *s)
{
    const struct runtime_valuevec *values = &s->values;
    struct runtime_value val = {0};

    if (runtime_queue_empty(s))
        return val;

    int len = runtime_valuevec_len(values);
    return runtime_valuevec_get(values, len - 1);
}
*/

/* No index range check */
/*
struct runtime_value runtime_queue_get(const struct runtime_queue *s, int64_t idx)
{
    return runtime_valuevec_get(&s->values, idx);
}
*/
