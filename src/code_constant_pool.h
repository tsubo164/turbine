#ifndef CODE_CONSTANT_POOL_H
#define CODE_CONSTANT_POOL_H

#include "value.h"

struct code_constant_pool {
    struct ValueVec ints;
    struct ValueVec floats;
    struct ValueVec strings;
};

void code_constant_pool_init(struct code_constant_pool *v);
void code_constant_pool_free(struct code_constant_pool *v);

int code_constant_pool_push_int(struct code_constant_pool *v, value_int_t val);
int code_constant_pool_push_float(struct code_constant_pool *v, value_float_t val);
int code_constant_pool_push_string(struct code_constant_pool *v, const char *val);

struct Value code_constant_pool_get_int(const struct code_constant_pool *v, int id);
struct Value code_constant_pool_get_float(const struct code_constant_pool *v, int id);
struct Value code_constant_pool_get_string(const struct code_constant_pool *v, int id);

int code_constant_pool_get_int_count(const struct code_constant_pool *v);
int code_constant_pool_get_float_count(const struct code_constant_pool *v);
int code_constant_pool_get_string_count(const struct code_constant_pool *v);

#endif /* _H */
