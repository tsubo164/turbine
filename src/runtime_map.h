#ifndef RUNTIME_MAP_H
#define RUNTIME_MAP_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct runtime_map {
    struct runtime_object obj;
    struct runtime_valuevec values;
};

struct runtime_map *runtime_map_new(int64_t len);
void runtime_map_free(struct runtime_map *m);

struct runtime_value runtime_map_get(const struct runtime_map *m, const char *key);
void runtime_map_set(struct runtime_map *m, const char *key, struct runtime_value val);

int64_t runtime_map_len(const struct runtime_map *m);
void runtime_map_resize(struct runtime_map *m, int64_t new_len);

#endif // _H
