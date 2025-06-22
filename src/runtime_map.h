#ifndef RUNTIME_MAP_H
#define RUNTIME_MAP_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct runtime_map_entry {
    struct runtime_value key;
    struct runtime_value val;
    struct runtime_map_entry *next_in_chain;
    struct runtime_map_entry *next_in_order;
};

struct runtime_map {
    struct runtime_object obj;
    struct runtime_map_entry **buckets;
    int val_type;

    int32_t prime_index;
    value_int_t cap;
    value_int_t len;

    struct runtime_map_entry head;
    struct runtime_map_entry *tail;
};

struct runtime_map *runtime_map_new(int val_type, value_int_t len);
void runtime_map_free(struct runtime_map *m);

struct runtime_value runtime_map_get(const struct runtime_map *m, struct runtime_value key);
void runtime_map_set(struct runtime_map *m, struct runtime_value key, struct runtime_value val);

value_int_t runtime_map_len(const struct runtime_map *m);

/* iteration */
struct runtime_map_entry *runtime_map_entry_begin(const struct runtime_map *m);
struct runtime_map_entry *runtime_map_entry_next(const struct runtime_map_entry *ent);

#endif /* _H */
