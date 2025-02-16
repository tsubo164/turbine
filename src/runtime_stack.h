#ifndef RUNTIME_STACK_H
#define RUNTIME_STACK_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct runtime_stack {
    struct runtime_object obj;
    struct runtime_valuevec values;
    int val_type;

    compare_function_t compare;
};

struct runtime_stack *runtime_stack_new(int val_type, int64_t len);
void runtime_stack_free(struct runtime_stack *s);

int64_t runtime_stack_len(const struct runtime_stack *s);
bool runtime_stack_empty(const struct runtime_stack *s);
void runtime_stack_push(struct runtime_stack *s, struct runtime_value val);
struct runtime_value runtime_stack_pop(struct runtime_stack *s);
struct runtime_value runtime_stack_top(const struct runtime_stack *s);

/* iteration */
/*
struct runtime_stack_node *runtime_stack_node_begin(const struct runtime_stack *s);
struct runtime_stack_node *runtime_stack_node_next(const struct runtime_stack_node *n);
*/

#endif /* _H */
