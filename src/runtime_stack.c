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
