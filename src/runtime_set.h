#ifndef RUNTIME_SET_H
#define RUNTIME_SET_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct set_node {
    struct runtime_value value;
    int height;
    struct set_node *l, *r;
};

struct runtime_set {
    struct runtime_object obj;
    struct set_node *root;
    int len;
};

struct runtime_set *runtime_set_new(int64_t len);
void runtime_set_free(struct runtime_set *s);

int64_t runtime_set_len(const struct runtime_set *s);
/*
struct runtime_value runtime_set_get(const struct runtime_set *s, struct runtime_value key);
void runtime_set_set(struct runtime_set *s, struct runtime_value key, struct runtime_value val);

*/

/* iteration */
/*
struct runtime_set_entry *runtime_set_entry_begin(const struct runtime_set *s);
struct runtime_set_entry *runtime_set_entry_next(const struct runtime_set_entry *ent);
*/

#endif /* _H */
