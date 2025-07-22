#ifndef RUNTIME_STACK_H
#define RUNTIME_STACK_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdbool.h>

struct runtime_stack {
    struct runtime_object obj;
    struct runtime_valuevec values;
    int val_type;

    compare_function_t compare;
};

struct runtime_stack *runtime_stack_new(struct runtime_gc *gc, int val_type, value_int_t len);
void runtime_stack_free(struct runtime_gc *gc, struct runtime_stack *s);

value_int_t runtime_stack_len(const struct runtime_stack *s);
bool runtime_stack_empty(const struct runtime_stack *s);
struct runtime_value runtime_stack_top(const struct runtime_stack *s);

/* gc managed */
void runtime_stack_push(struct runtime_gc *gc, struct runtime_stack *s, struct runtime_value val);
struct runtime_value runtime_stack_pop(struct runtime_gc *gc, struct runtime_stack *s);

/* no index range check */
struct runtime_value runtime_stack_get(const struct runtime_stack *s, value_int_t idx);

#endif /* _H */
