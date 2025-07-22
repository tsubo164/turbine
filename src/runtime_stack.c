#include "runtime_stack.h"
#include <stdlib.h>
#include <stdio.h>

struct runtime_stack *runtime_stack_new(struct runtime_gc *gc, int val_type, value_int_t len)
{
    struct runtime_stack *s;

    s = runtime_alloc_object2(gc, OBJ_STACK, sizeof(*s));
    s->val_type = val_type;
    runtime_valuevec_init(&s->values);

    runtime_gc_push_object(gc, (struct runtime_object*) s);
    return s;
}

void runtime_stack_free(struct runtime_gc *gc, struct runtime_stack *s)
{
    if (!s)
        return;
    runtime_valuevec_free(gc, &s->values);
    runtime_gc_free(gc, s);
}

value_int_t runtime_stack_len(const struct runtime_stack *s)
{
    return runtime_valuevec_len(&s->values);
}

bool runtime_stack_empty(const struct runtime_stack *s)
{
    return runtime_valuevec_len(&s->values) == 0;
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

/* gc managed */
void runtime_stack_push(struct runtime_gc *gc, struct runtime_stack *s, struct runtime_value val)
{
    runtime_valuevec_push(gc, &s->values, val);
}

struct runtime_value runtime_stack_pop(struct runtime_gc *gc, struct runtime_stack *s)
{
    struct runtime_valuevec *values = &s->values;
    struct runtime_value val = {0};

    if (runtime_stack_empty(s))
        return val;

    int len = runtime_valuevec_len(values);
    val = runtime_valuevec_get(values, len - 1);
    runtime_valuevec_resize(gc, values, len - 1);

    return val;
}

/* no index range check */
struct runtime_value runtime_stack_get(const struct runtime_stack *s, value_int_t idx)
{
    return runtime_valuevec_get(&s->values, idx);
}
