#ifndef CODE_CONSTANT_POOL_H
#define CODE_CONSTANT_POOL_H

#include "runtime_value.h"

struct code_constant {
    struct runtime_value val;
    int type;
};

/* constant pool */
struct code_constant_pool {
    struct code_constant *data;
    int len;
    int cap;
};

void code_constant_pool_init(struct code_constant_pool *pool);
void code_constant_pool_clear(struct code_constant_pool *pool);

int code_constant_pool_push_int(struct code_constant_pool *pool, value_int_t val);
int code_constant_pool_push_float(struct code_constant_pool *pool, value_float_t val);
int code_constant_pool_push_string(struct code_constant_pool *pool, const char *val);

int code_constant_pool_get_count(const struct code_constant_pool *pool);
struct runtime_value code_constant_pool_get(const struct code_constant_pool *pool, int id);
int code_constant_pool_get_type(const struct code_constant_pool *pool, int id);

/* literal table */
struct code_literal_table {
    struct code_constant *data;
    int len;
    int cap;
};

void code_literal_table_init(struct code_literal_table *table);
void code_literal_table_clear(struct code_literal_table *table);

int code_literal_table_push_int(struct code_literal_table *table, value_int_t val);
int code_literal_table_push_float(struct code_literal_table *table, value_float_t val);
int code_literal_table_push_string(struct code_literal_table *table, const char *val);

int code_literal_table_get_count(const struct code_literal_table *table);
struct runtime_value code_literal_table_get(const struct code_literal_table *table, int id);
int code_literal_table_get_type(const struct code_literal_table *table, int id);

#endif /* _H */
