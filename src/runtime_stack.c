#include "runtime_stack.h"
#include <stdlib.h>
#include <stdio.h>

struct runtime_stack *runtime_stack_new(int val_type, int64_t len)
{
    struct runtime_stack *s;

    s = calloc(1, sizeof(*s));
    s->obj.kind = OBJ_SET;
    s->val_type = val_type;
    s->compare = runtime_get_compare_function(s->val_type);

    return s;
}

void runtime_stack_free(struct runtime_stack *s)
{
    if (!s)
        return;
    runtime_valuevec_free(&s->values);
    free(s);
}

int64_t runtime_stack_len(const struct runtime_stack *s)
{
    return runtime_valuevec_len(&s->values);
}

bool runtime_stack_empty(const struct runtime_stack *s)
{
    return runtime_valuevec_len(&s->values) == 0;
}

void runtime_stack_push(struct runtime_stack *s, struct runtime_value val)
{
    runtime_valuevec_push(&s->values, val);
}

struct runtime_value runtime_stack_pop(struct runtime_stack *s)
{
    struct runtime_valuevec *values = &s->values;
    struct runtime_value val = {0};

    if (runtime_stack_empty(s))
        return val;

    int len = runtime_valuevec_len(values);
    val = runtime_valuevec_get(values, len - 1);
    runtime_valuevec_resize(values, len - 1);

    return val;
}

struct runtime_value runtime_stack_top(const struct runtime_stack *s)
{
    const struct runtime_valuevec *values = &s->values;
    struct runtime_value val = {0};

    if (runtime_stack_empty(s))
        return val;

    int len = runtime_valuevec_len(values);
    return runtime_valuevec_get(values, len - 1);
}

/* No index range check */
struct runtime_value runtime_stack_get(const struct runtime_stack *s, int64_t idx)
{
    return runtime_valuevec_get(&s->values, idx);
}
