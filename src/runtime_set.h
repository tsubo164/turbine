#ifndef RUNTIME_SET_H
#define RUNTIME_SET_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct set_node {
    struct runtime_value val;
    struct set_node *l, *r;
    int height;
};

struct runtime_set {
    struct runtime_object obj;
    struct set_node *root;
    int len;
};

struct runtime_set *runtime_set_new(int64_t len);
void runtime_set_free(struct runtime_set *s);

int64_t runtime_set_len(const struct runtime_set *s);
bool runtime_set_add(struct runtime_set *s, struct runtime_value val);
bool runtime_set_remove(struct runtime_set *s, struct runtime_value val);
bool runtime_set_contains(const struct runtime_set *s, struct runtime_value val);

/* iteration */
/*
struct runtime_set_entry *runtime_set_entry_begin(const struct runtime_set *s);
struct runtime_set_entry *runtime_set_entry_next(const struct runtime_set_entry *ent);
*/

void runtime_print_set_tree(const struct runtime_set *s);

#endif /* _H */
