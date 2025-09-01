#ifndef CODE_CONSTANT_POOL_H
#define CODE_CONSTANT_POOL_H

#include "runtime_value.h"
#include "data_strbuf.h"

struct code_constant {
    struct runtime_value val;
    int type;
};

struct code_constant_pool {
    /* TODO consider making struct code_literal_table */
    struct runtime_valuevec literals;
    struct data_strbuf literal_types;

    struct code_constant *data;
    int len;
    int cap;
};

void code_constant_pool_init(struct code_constant_pool *pool);
void code_constant_pool_free(struct code_constant_pool *pool);

int code_constant_pool_push_int(struct code_constant_pool *pool, value_int_t val);
int code_constant_pool_push_float(struct code_constant_pool *pool, value_float_t val);
int code_constant_pool_push_string(struct code_constant_pool *pool, const char *val);

int code_constant_pool_get_count(const struct code_constant_pool *pool);
struct runtime_value code_constant_pool_get(const struct code_constant_pool *pool, int id);
int code_constant_pool_get_type(const struct code_constant_pool *pool, int id);

int code_constant_pool_push_literal_int(struct code_constant_pool *pool, value_int_t val);
int code_constant_pool_push_literal_float(struct code_constant_pool *pool, value_float_t val);
int code_constant_pool_push_literal_string(struct code_constant_pool *pool, const char *val);
struct runtime_value code_constant_pool_get_literal(const struct code_constant_pool *pool, int id);

bool code_constant_pool_is_literal_int(const struct code_constant_pool *pool, int id);
bool code_constant_pool_is_literal_float(const struct code_constant_pool *pool, int id);
bool code_constant_pool_is_literal_string(const struct code_constant_pool *pool, int id);
int code_constant_pool_get_literal_count(const struct code_constant_pool *pool);

#endif /* _H */
