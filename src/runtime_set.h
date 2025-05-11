#ifndef RUNTIME_SET_H
#define RUNTIME_SET_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct runtime_set_node {
    struct runtime_value val;
    struct runtime_set_node *l, *r;
    struct runtime_set_node *parent;
    int height;
};

struct runtime_set {
    struct runtime_object obj;
    struct runtime_set_node *root;
    struct runtime_set_node *min;
    int val_type;
    value_int_t len;

    compare_function_t compare;
};

struct runtime_set *runtime_set_new(int val_type, value_int_t len);
void runtime_set_free(struct runtime_set *s);

value_int_t runtime_set_len(const struct runtime_set *s);
bool runtime_set_add(struct runtime_set *s, struct runtime_value val);
bool runtime_set_remove(struct runtime_set *s, struct runtime_value val);
bool runtime_set_contains(const struct runtime_set *s, struct runtime_value val);

/* iteration */
struct runtime_set_node *runtime_set_node_begin(const struct runtime_set *s);
struct runtime_set_node *runtime_set_node_next(const struct runtime_set_node *n);

void runtime_print_set_tree(const struct runtime_set *s);

#endif /* _H */
