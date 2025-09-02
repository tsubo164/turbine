#include "code_constant_pool.h"
#include "runtime_string.h"
#include <assert.h>

#define MIN_CAP 8

void code_constant_pool_init(struct code_constant_pool *pool)
{
    pool->data = NULL;
    pool->cap = 0;
    pool->len = 0;
}

void code_constant_pool_clear(struct code_constant_pool *pool)
{
    /* free const strings */
    for (int i = 0; i < pool->len; i++) {
        if (pool->data[i].type != VAL_STRING)
            continue;
        struct runtime_value val = pool->data[i].val;
        runtime_string_free(NULL, val.string);
    }

    free(pool->data);
    code_constant_pool_init(pool);
}

static void push_const(struct code_constant_pool *p, struct code_constant c)
{
    if (p->len == p->cap) {
        p->cap = p->cap < MIN_CAP ? MIN_CAP : 2 * p->cap;
        p->data = realloc(p->data, p->cap * sizeof(*p->data));
    }
    p->data[p->len++] = c;
}

int code_constant_pool_push_int(struct code_constant_pool *pool, value_int_t val)
{
    /* find */
    for (int i = 0; i < pool->len; i++) {
        if (pool->data[i].type != VAL_INT)
            continue;
        if (pool->data[i].val.inum == val)
            return i;
    }

    int new_idx = pool->len;
    struct runtime_value v = {.inum = val};
    struct code_constant c = {.val = v, .type = VAL_INT};

    push_const(pool, c);
    return new_idx;
}

int code_constant_pool_push_float(struct code_constant_pool *pool, value_float_t val)
{
    /* find */
    for (int i = 0; i < pool->len; i++) {
        if (pool->data[i].type != VAL_FLOAT)
            continue;
        if (pool->data[i].val.fpnum == val)
            return i;
    }

    int new_idx = pool->len;
    struct runtime_value v = {.fpnum = val};
    struct code_constant c = {.val = v, .type = VAL_FLOAT};

    push_const(pool, c);
    return new_idx;
}

int code_constant_pool_push_string(struct code_constant_pool *pool, const char *val)
{
    /* find */
    for (int i = 0; i < pool->len; i++) {
        if (pool->data[i].type != VAL_STRING)
            continue;
        struct runtime_value s = pool->data[i].val;
        if (runtime_string_compare_cstr(s.string, val) == 0)
            return i;
    }

    int new_idx = pool->len;
    struct runtime_value v = {.string = runtime_string_new(NULL, val)};
    struct code_constant c = {.val = v, .type = VAL_STRING};

    push_const(pool, c);
    return new_idx;
}

int code_constant_pool_get_count(const struct code_constant_pool *pool)
{
    return pool->len;
}

struct runtime_value code_constant_pool_get(const struct code_constant_pool *pool, int id)
{
    assert(id >= 0 && id < pool->len);
    return pool->data[id].val;
}

int code_constant_pool_get_type(const struct code_constant_pool *pool, int id)
{
    assert(id >= 0 && id < pool->len);
    return pool->data[id].type;
}

/* literal table */
void code_literal_table_init(struct code_literal_table *table)
{
    table->data = NULL;
    table->cap = 0;
    table->len = 0;
}

void code_literal_table_clear(struct code_literal_table *table)
{
    /* free const strings */
    for (int i = 0; i < table->len; i++) {
        if (table->data[i].type != VAL_STRING)
            continue;
        struct runtime_value val = table->data[i].val;
        runtime_string_free(NULL, val.string);
    }

    free(table->data);
    code_literal_table_init(table);
}

static void push_literal(struct code_literal_table *t, struct code_constant c)
{
    if (t->len == t->cap) {
        t->cap = t->cap < MIN_CAP ? MIN_CAP : 2 * t->cap;
        t->data = realloc(t->data, t->cap * sizeof(*t->data));
    }
    t->data[t->len++] = c;
}

int code_literal_table_push_int(struct code_literal_table *table, value_int_t val)
{
    int new_idx = table->len;
    struct runtime_value v = {.inum = val};
    struct code_constant c = {.val = v, .type = VAL_INT};

    push_literal(table, c);
    return new_idx;
}

int code_literal_table_push_float(struct code_literal_table *table, value_float_t val)
{
    int new_idx = table->len;
    struct runtime_value v = {.fpnum = val};
    struct code_constant c = {.val = v, .type = VAL_FLOAT};

    push_literal(table, c);
    return new_idx;
}

int code_literal_table_push_string(struct code_literal_table *table, const char *val)
{
    int new_idx = table->len;
    struct runtime_value v = {.string = runtime_string_new(NULL, val)};
    struct code_constant c = {.val = v, .type = VAL_STRING};

    push_literal(table, c);
    return new_idx;
}

int code_literal_table_get_count(const struct code_literal_table *table)
{
    return table->len;
}

struct runtime_value code_literal_table_get(const struct code_literal_table *table, int id)
{
    assert(id >= 0 && id < table->len);
    return table->data[id].val;
}

int code_literal_table_get_type(const struct code_literal_table *table, int id)
{
    assert(id >= 0 && id < table->len);
    return table->data[id].type;
}
