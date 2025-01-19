#ifndef CODE_CONSTANT_POOL_H
#define CODE_CONSTANT_POOL_H

#include "runtime_value.h"
#include "data_strbuf.h"

struct code_constant_pool {
    struct runtime_valuevec ints;
    struct runtime_valuevec floats;
    struct runtime_valuevec strings;

    /* TODO consider making struct code_literal_table */
    struct runtime_valuevec literals;
    struct data_strbuf literal_types;
};

void code_constant_pool_init(struct code_constant_pool *v);
void code_constant_pool_free(struct code_constant_pool *v);

int code_constant_pool_push_int(struct code_constant_pool *v, value_int_t val);
int code_constant_pool_push_float(struct code_constant_pool *v, value_float_t val);
int code_constant_pool_push_string(struct code_constant_pool *v, const char *val);

struct runtime_value code_constant_pool_get_int(const struct code_constant_pool *v, int id);
struct runtime_value code_constant_pool_get_float(const struct code_constant_pool *v, int id);
struct runtime_value code_constant_pool_get_string(const struct code_constant_pool *v, int id);

int code_constant_pool_get_int_count(const struct code_constant_pool *v);
int code_constant_pool_get_float_count(const struct code_constant_pool *v);
int code_constant_pool_get_string_count(const struct code_constant_pool *v);

int code_constant_pool_push_literal_int(struct code_constant_pool *v, value_int_t val);
int code_constant_pool_push_literal_string(struct code_constant_pool *v, const char *val);
struct runtime_value code_constant_pool_get_literal(const struct code_constant_pool *v, int id);

bool code_constant_pool_is_literal_int(const struct code_constant_pool *v, int id);
bool code_constant_pool_is_literal_string(const struct code_constant_pool *v, int id);
int code_constant_pool_get_literal_count(const struct code_constant_pool *v);

#endif /* _H */
